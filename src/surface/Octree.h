#pragma once
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <new>
#include <algorithm>
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
	uint8_t cornerSigns = 0;
	OctreeNode* children[8]{};
	bool isLeaf = true;

	OctreeNode(const AABB& box) : bounds(box)
	{
		for (int i = 0; i < 8; ++i) children[i] = nullptr;
	}
};

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

        // --- KEY FIX: cache resolution follows minCellSize ---
        m_CacheInvStep = 1.0f / (0.5f * m_MinCellSize);

        m_CacheNx = (uint32_t)std::max(1, (int)std::ceil(sz.x * m_CacheInvStep));
        m_CacheNy = (uint32_t)std::max(1, (int)std::ceil(sz.y * m_CacheInvStep));
        m_CacheNz = (uint32_t)std::max(1, (int)std::ceil(sz.z * m_CacheInvStep));

        m_EvalCache.reserve(std::min<size_t>(m_PoolSize * 2ull, 2'000'000ull));
    }

    ~Octree() { std::free(m_Pool); }

    bool Exhausted() const { return m_Exhausted; }

    template<typename F>
    void Subdivide(OctreeNode* node, const F& f, int depthLeft)
    {
        if (!node) return;

        const glm::vec3 mn   = node->bounds.min;
        const glm::vec3 mx   = node->bounds.max;
        const glm::vec3 c    = node->bounds.center();
        const glm::vec3 size = mx - mn;

        // --- FIXED evalSafe: uses minCellSize-based grid ---
        auto evalSafe = [&](float x, float y, float z, bool& ok) -> float
        {
            ok = true;

            int ix = (int)std::floor((x - m_RootMin.x) * m_CacheInvStep);
            int iy = (int)std::floor((y - m_RootMin.y) * m_CacheInvStep);
            int iz = (int)std::floor((z - m_RootMin.z) * m_CacheInvStep);

            ix = std::clamp(ix, 0, (int)m_CacheNx - 1);
            iy = std::clamp(iy, 0, (int)m_CacheNy - 1);
            iz = std::clamp(iz, 0, (int)m_CacheNz - 1);

            const uint64_t key = PackKey(ix, iy, iz);

            auto it = m_EvalCache.find(key);
            if (it != m_EvalCache.end())
                return it->second;

            float v = 0.0f;
            try {
                v = (float)f(x, y, z);
            } catch (...) {
                ok = false;
                return 0.0f;
            }

            if (!std::isfinite(v)) { ok = false; return 0.0f; }

            m_EvalCache.emplace(key, v);
            return v;
        };

        node->cornerSigns = 0;
        bool cornersFinite = true;

        // Evaluate corners
        for (int i = 0; i < 8; ++i)
        {
            glm::vec3 p = mn + CORNER_OFFSETS[i] * size;

            bool ok = true;
            float v = evalSafe(p.x, p.y, p.z, ok);

            node->cornerValues[i] = v;
            if (!ok) cornersFinite = false;
            if (ok && v < 0.0f) node->cornerSigns |= (1 << i);
        }

        const bool homogeneous = (node->cornerSigns == 0x00 || node->cornerSigns == 0xFF);

        const bool canSubdivide =
            depthLeft > 0 &&
            size.x > m_MinCellSize &&
            size.y > m_MinCellSize &&
            size.z > m_MinCellSize;

        if (!canSubdivide)
        {
            node->isLeaf = true;
            return;
        }

        bool mustSubdivide = !homogeneous;

        if (homogeneous)
        {
            float vmin =  1e30f;
            float vmax = -1e30f;

            // Corner mins/maxs
            for (int i = 0; i < 8; i++)
            {
                float v = node->cornerValues[i];
                vmin = std::min(vmin, v);
                vmax = std::max(vmax, v);
            }

            // --- FIX: denser probing (gyroid interior detection) ---
            auto probe = [&](const glm::vec3& p)
            {
                bool ok = true;
                float v = evalSafe(p.x, p.y, p.z, ok);
                if (!ok) { cornersFinite = false; return; }
                vmin = std::min(vmin, v);
                vmax = std::max(vmax, v);
            };

            probe(c);

            float x0 = mn.x, x1 = mx.x;
            float y0 = mn.y, y1 = mx.y;
            float z0 = mn.z, z1 = mx.z;
            float xm = (x0 + x1) * 0.5f;
            float ym = (y0 + y1) * 0.5f;
            float zm = (z0 + z1) * 0.5f;

            // Centers of faces — enough to catch gyroid oscillation
            probe({x0, ym, zm}); probe({x1, ym, zm});
            probe({xm, y0, zm}); probe({xm, y1, zm});
            probe({xm, ym, z0}); probe({xm, ym, z1});

            bool crossesIso = (vmin <= 0.0f && vmax >= 0.0f);

            int levelFromRoot = m_MaxDepth - depthLeft;

            if (levelFromRoot < m_ForceLevels)
                mustSubdivide = true;
            else
                mustSubdivide = crossesIso;

            if (!cornersFinite) mustSubdivide = true;
        }

        if (!mustSubdivide)
        {
            node->isLeaf = true;
            return;
        }

        node->isLeaf = false;

        // Spawn child nodes
        for (int i = 0; i < 8; ++i)
        {
            glm::vec3 childMin{
                (i & 1) ? c.x : mn.x,
                (i & 2) ? c.y : mn.y,
                (i & 4) ? c.z : mn.z
            };

            glm::vec3 childMax{
                (i & 1) ? mx.x : c.x,
                (i & 2) ? mx.y : c.y,
                (i & 4) ? mx.z : c.z
            };

            node->children[i] = alloc({ childMin, childMax });
            if (!node->children[i])
            {
                node->isLeaf = true;
                return;
            }

            Subdivide(node->children[i], f, depthLeft - 1);
        }
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
        return (Pow8(depth + 1) - 1) / 7;
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

    static uint64_t PackKey(uint32_t ix, uint32_t iy, uint32_t iz)
    {
        return (uint64_t)(ix & 0x1FFFFF)
            | ((uint64_t)(iy & 0x1FFFFF) << 21)
            | ((uint64_t)(iz & 0x1FFFFF) << 42);
    }

    glm::vec3 m_RootMin{};
    std::unordered_map<uint64_t, float> m_EvalCache;
};