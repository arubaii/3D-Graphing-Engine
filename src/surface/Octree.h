#pragma once
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <new>
#include <algorithm>
#include <functional>
#include <glm/glm.hpp>
#include <unordered_map>
#include "utils/Log.h"

constexpr int   MAX_OCTREE_DEPTH = 8;
constexpr float MIN_CELL_SIZE    = 0.1f;
constexpr float ISO_EPSILON      = 1e-3f;


static constexpr glm::vec3 CORNER_OFFSETS[8] = {
	{0,0,0},
	{1,0,0},
	{1,1,0},
	{0,1,0},
	{0,0,1},
	{1,0,1},
	{1,1,1},
	{0,1,1}
};


struct AABB
{
	glm::vec3 min;
	glm::vec3 max;

	AABB() = default;
	AABB(const glm::vec3& mn, const glm::vec3& mx) : min(mn), max(mx) {}

	glm::vec3 center() const { return 0.5f * (min + max); }
	glm::vec3 size()   const { return max - min; }
};


struct OctreeNode
{
	AABB bounds;
	float cornerValues[8]{};
	uint8_t cornerSigns = 0;  // 0x00 = all positive, 0xFF = all negative, else mixed
	OctreeNode* children[8]{};
	bool isLeaf = true;

	OctreeNode(const AABB& box) : bounds(box)
	{
		for (int i = 0; i < 8; ++i) children[i] = nullptr;
	}
};

// Adaptive octree that subdivides based on sign changes and proximity to the isosurface.
// Uses a flat memory pool to avoid per-node heap allocation, and caches field evaluations
// on a regular grid to avoid redundant calls at shared corners.
class Octree
{
public:
    OctreeNode* root = nullptr;

    explicit Octree(const AABB& bounds, int maxDepth, float minCellSize, int forceLevels = 3)
    {
        m_MaxDepth    = std::clamp(maxDepth, 1, MAX_OCTREE_DEPTH);
        m_MinCellSize = std::max(minCellSize, 1e-6f);
        m_ForceLevels = std::max(0, forceLevels);

        m_PoolSize = ComputePoolSize(m_MaxDepth);
        m_Pool = (OctreeNode*)std::malloc(sizeof(OctreeNode) * m_PoolSize);
        m_Next = 0;

        root = alloc(bounds);
        m_RootMin = bounds.min;

        glm::vec3 sz = bounds.size();

        // Cache grid resolution tied to half a min cell so shared corners land in the same bucket
        m_CacheInvStep = 1.0f / (0.5f * m_MinCellSize);

        m_CacheNx = (uint32_t)std::max(1, (int)std::ceil(sz.x * m_CacheInvStep));
        m_CacheNy = (uint32_t)std::max(1, (int)std::ceil(sz.y * m_CacheInvStep));
        m_CacheNz = (uint32_t)std::max(1, (int)std::ceil(sz.z * m_CacheInvStep));

        m_EvalCache.reserve(std::min<size_t>(m_PoolSize * 2ull, 2'000'000ull));
    }

    ~Octree() { std::free(m_Pool); }

    bool Exhausted() const { return m_Exhausted; }

    void Subdivide(OctreeNode* node,
                   const std::function<float(float,float,float)>& field,
                   int depth)
    {
        if (!node) return;
        if (m_Exhausted) return;

        const glm::vec3 mn = node->bounds.min;
        const glm::vec3 mx = node->bounds.max;
        const glm::vec3 sz = node->bounds.size();

        glm::vec3 corners[8] =
        {
            {mn.x, mn.y, mn.z},
            {mx.x, mn.y, mn.z},
            {mx.x, mx.y, mn.z},
            {mn.x, mx.y, mn.z},
            {mn.x, mn.y, mx.z},
            {mx.x, mn.y, mx.z},
            {mx.x, mx.y, mx.z},
            {mn.x, mx.y, mx.z}
        };

        uint8_t mask = 0;
        float vmin =  1e30f;
        float vmax = -1e30f;
        float minAbs = 1e30f;

        for (int i = 0; i < 8; ++i)
        {
            float v = EvalCached(field, corners[i].x, corners[i].y, corners[i].z);
            node->cornerValues[i] = v;
            if (v < 0.0f) mask |= (1u << i);

            vmin = std::min(vmin, v);
            vmax = std::max(vmax, v);
            minAbs = std::min(minAbs, std::abs(v));
        }

        node->cornerSigns = (mask == 0xFF) ? 0xFF : (mask == 0x00 ? 0x00 : mask);

        if (depth <= 0)
        {
            node->isLeaf = true;
            return;
        }

        // Force subdivision near the root regardless of sign to catch thin features
        const bool forced = (depth > (m_MaxDepth - m_ForceLevels));

        const bool signChangeAtCorners = (mask != 0x00 && mask != 0xFF);
        bool shouldSplit = forced || signChangeAtCorners;

        if (!shouldSplit)
        {
            // Corners are uniform sign — check interior for hidden sign flips
            const bool cornerNeg = (mask == 0xFF);
            if (HasInteriorSignFlip(field, mn, mx, cornerNeg))
            {
                shouldSplit = true;
            }
            else
            {
                // Approximate distance-to-surface via gradient magnitude;
                // split if the surface likely passes within 75% of the cell diagonal
                const glm::vec3 c = (mn + mx) * 0.5f;
                float vc = EvalCached(field, c.x, c.y, c.z);

                float h = 0.25f * std::min(sz.x, std::min(sz.y, sz.z));
                h = std::max(h, 1e-4f);

                float fx1 = EvalCached(field, c.x + h, c.y,     c.z);
                float fx0 = EvalCached(field, c.x - h, c.y,     c.z);
                float fy1 = EvalCached(field, c.x,     c.y + h, c.z);
                float fy0 = EvalCached(field, c.x,     c.y - h, c.z);
                float fz1 = EvalCached(field, c.x,     c.y,     c.z + h);
                float fz0 = EvalCached(field, c.x,     c.y,     c.z - h);

                glm::vec3 g(fx1 - fx0, fy1 - fy0, fz1 - fz0);
                float gradMag = glm::length(g) / (2.0f * h);
                gradMag = std::max(gradMag, 1e-6f);

                float approxDist = std::abs(vc) / gradMag;
                float cellDiag = glm::length(sz);

                if (approxDist < 0.75f * cellDiag)
                    shouldSplit = true;
            }
        }

        if (!shouldSplit)
        {
            node->isLeaf = true;
            return;
        }

        node->isLeaf = false;

        const glm::vec3 half = sz * 0.5f;

        for (int i = 0; i < 8; ++i)
        {
            glm::vec3 o = CORNER_OFFSETS[i] * half;
            AABB childBounds(mn + o, mn + o + half);

            OctreeNode* child = AllocateNode(childBounds);
            if (!child)
            {
                m_Exhausted = true;
                return;
            }

            node->children[i] = child;
        }

        for (int i = 0; i < 8; ++i)
            Subdivide(node->children[i], field, depth - 1);
    }

    // Probes face centers, edge midpoints, and cell center to detect sign flips
    // that corner sampling alone would miss (e.g. thin sheets, small enclosed regions)
    bool HasInteriorSignFlip(const std::function<float(float,float,float)>& field,
                         const glm::vec3& mn,
                         const glm::vec3& mx,
                         bool cornerNegative)
    {
        glm::vec3 c = (mn + mx) * 0.5f;

        glm::vec3 e[12] = {
            {c.x, mn.y, mn.z}, {mx.x, c.y, mn.z}, {c.x, mx.y, mn.z}, {mn.x, c.y, mn.z},
            {c.x, mn.y, mx.z}, {mx.x, c.y, mx.z}, {c.x, mx.y, mx.z}, {mn.x, c.y, mx.z},
            {mn.x, mn.y, c.z}, {mx.x, mn.y, c.z}, {mx.x, mx.y, c.z}, {mn.x, mx.y, c.z}
        };

        glm::vec3 f[6] = {
            {c.x, c.y, mn.z}, {c.x, c.y, mx.z},
            {c.x, mn.y, c.z}, {c.x, mx.y, c.z},
            {mn.x, c.y, c.z}, {mx.x, c.y, c.z}
        };

        auto opp = [&](float v) { return (v < 0.0f) != cornerNegative; };

        float vc = EvalCached(field, c.x, c.y, c.z);
        if (opp(vc)) return true;

        for (int i = 0; i < 12; ++i) {
            float v = EvalCached(field, e[i].x, e[i].y, e[i].z);
            if (opp(v)) return true;
        }

        for (int i = 0; i < 6; ++i) {
            float v = EvalCached(field, f[i].x, f[i].y, f[i].z);
            if (opp(v)) return true;
        }

        return false;
    }

    // Snaps world position to a cache grid cell and returns a memoized field value,
    // avoiding redundant evaluations at shared octree corners
    float EvalCached(const std::function<float(float,float,float)>& field, float x, float y, float z)
    {
        if (m_Exhausted) return 1e30f;

        float fx = (x - m_RootMin.x) * m_CacheInvStep;
        float fy = (y - m_RootMin.y) * m_CacheInvStep;
        float fz = (z - m_RootMin.z) * m_CacheInvStep;

        int ix = (int)std::floor(fx);
        int iy = (int)std::floor(fy);
        int iz = (int)std::floor(fz);

        if (ix < 0) ix = 0;
        if (iy < 0) iy = 0;
        if (iz < 0) iz = 0;

        if (ix >= (int)m_CacheNx) ix = (int)m_CacheNx - 1;
        if (iy >= (int)m_CacheNy) iy = (int)m_CacheNy - 1;
        if (iz >= (int)m_CacheNz) iz = (int)m_CacheNz - 1;

        uint64_t key = PackKey((uint32_t)ix, (uint32_t)iy, (uint32_t)iz);

        auto it = m_EvalCache.find(key);
        if (it != m_EvalCache.end())
            return it->second;

        // Evaluate at cell center rather than exact position to maximise cache reuse
        float step = 1.0f / m_CacheInvStep;
        float sx = m_RootMin.x + (float(ix) + 0.5f) * step;
        float sy = m_RootMin.y + (float(iy) + 0.5f) * step;
        float sz = m_RootMin.z + (float(iz) + 0.5f) * step;

        float v = field(sx, sy, sz);
        if (!std::isfinite(v)) v = std::copysign(1e30f, v);

        m_EvalCache.emplace(key, v);
        return v;
    }

    OctreeNode* AllocateNode(const AABB& b)
    {
        return alloc(b);
    }

private:
    int   m_MaxDepth     = 6;
    float m_MinCellSize  = 0.1f;
    int   m_ForceLevels  = 3;

    uint32_t m_CacheNx = 1, m_CacheNy = 1, m_CacheNz = 1;
    float    m_CacheInvStep = 1.0f;

    static constexpr size_t Pow8(int e)
    {
        size_t r = 1;
        for (int i = 0; i < e; ++i) r *= 8;
        return r;
    }

    static constexpr size_t MaxNodesForDepth(int depth)
    {
        return (Pow8(depth + 1) - 1) / 7;  // geometric series: sum of 8^0..8^depth
    }

    static constexpr size_t MAX_POOL_NODES = 8'000'000;

    static size_t ComputePoolSize(int depth)
    {
        size_t need = MaxNodesForDepth(depth);
        if (need > MAX_POOL_NODES) need = MAX_POOL_NODES;
        if (need < 1024) need = 1024;
        return need;
    }

    size_t m_PoolSize = 0;
    OctreeNode* m_Pool = nullptr;
    size_t m_Next = 0;
    bool m_Exhausted = false;

    OctreeNode* alloc(const AABB& b)
    {
        if (!m_Pool) { m_Exhausted = true; return nullptr; }
        if (m_Next >= m_PoolSize) { m_Exhausted = true; return nullptr; }
        return new (&m_Pool[m_Next++]) OctreeNode(b);
    }

    // Packs 3x 21-bit grid indices into a single 64-bit key
    static uint64_t PackKey(uint32_t ix, uint32_t iy, uint32_t iz)
    {
        return (uint64_t)(ix & 0x1FFFFF)
            | ((uint64_t)(iy & 0x1FFFFF) << 21)
            | ((uint64_t)(iz & 0x1FFFFF) << 42);
    }

    glm::vec3 m_RootMin{};
    std::unordered_map<uint64_t, float> m_EvalCache;
};