#include <unordered_map>
#include <cstdint>
#include <cmath>

#include "SurfaceMesh.h"
#include "utils/Log.h"
#include "MarchingCubes.h"


static inline uint64_t Pack21_3(int32_t x, int32_t y, int32_t z)
{
	// 21 bits each, signed via bias
	const uint32_t B = (1u << 20); // bias
	const uint64_t ux = (uint64_t)(uint32_t)(x + (int32_t)B) & 0x1FFFFFu;
	const uint64_t uy = (uint64_t)(uint32_t)(y + (int32_t)B) & 0x1FFFFFu;
	const uint64_t uz = (uint64_t)(uint32_t)(z + (int32_t)B) & 0x1FFFFFu;
	return (ux) | (uy << 21) | (uz << 42);
}

static inline uint64_t QuantKey(const glm::vec3& p, float step)
{
	// step must be > 0
	const float inv = 1.0f / step;
	const int32_t qx = (int32_t)llround((double)p.x * (double)inv);
	const int32_t qy = (int32_t)llround((double)p.y * (double)inv);
	const int32_t qz = (int32_t)llround((double)p.z * (double)inv);
	return Pack21_3(qx, qy, qz);
}

static inline uint32_t GetOrCreateVertex(SurfaceMesh& mesh, const glm::vec3& pos)
{
	if (mesh.m_QuantizeStep <= 0.0f)
	{
		Vertex v{};
		v.Position = pos;
		v.Normal   = glm::vec3(0.0f);

		const uint32_t idx = (uint32_t)mesh.m_CPU.Vertices.size();
		mesh.m_CPU.Vertices.push_back(v);
		return idx;
	}

	const float step = std::max(mesh.m_QuantizeStep, 1e-6f);
	const uint64_t key = QuantKey(pos, step);

	auto it = mesh.m_VertexCache.find(key);
	if (it != mesh.m_VertexCache.end())
		return it->second;

	Vertex v{};
	v.Position = pos;
	v.Normal   = glm::vec3(0.0f);

	const uint32_t idx = (uint32_t)mesh.m_CPU.Vertices.size();
	mesh.m_CPU.Vertices.push_back(v);
	mesh.m_VertexCache.emplace(key, idx);
	return idx;
}

static inline void AccumNormal(SurfaceMesh& mesh, uint32_t idx, const glm::vec3& n)
{
	mesh.m_CPU.Vertices[idx].Normal += n;
}

static inline void FinalizeNormals(SurfaceMesh& mesh)
{
	for (auto& v : mesh.m_CPU.Vertices)
	{
		const float len2 = glm::dot(v.Normal, v.Normal);
		if (len2 > 1e-20f)
			v.Normal *= (1.0f / std::sqrt(len2));
		else
			v.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
	}
}

static glm::vec3 Interp(
	const glm::vec3& p0, const glm::vec3& p1,
	float v0, float v1
)
{
	if (std::abs(v0 - v1) < 1e-6f)  // Prevent division by zero
		return p0;

	float t = v0 / (v0 - v1);
	t = glm::clamp(t, 0.0f, 1.0f);
	return p0 + t * (p1 - p0);
}

static glm::vec3 GradientCentral(
    const std::function<float(float,float,float)>& F,
    const glm::vec3& p,
    float h
)
{
    const float fx1 = F(p.x + h, p.y,     p.z);
    const float fx0 = F(p.x - h, p.y,     p.z);
    const float fy1 = F(p.x,     p.y + h, p.z);
    const float fy0 = F(p.x,     p.y - h, p.z);
    const float fz1 = F(p.x,     p.y,     p.z + h);
    const float fz0 = F(p.x,     p.y,     p.z - h);

    glm::vec3 g(fx1 - fx0, fy1 - fy0, fz1 - fz0);
    return g / (2.0f * h);
}

static void MarchingCubes(
    const AABB& cell,
    const float values[8],
    const std::function<float(float,float,float)>& F, // signed field
    SurfaceMesh& mesh
)
{
    // Compute cube index
    int cubeIndex = 0;
    for (int i = 0; i < 8; i++)
        if (values[i] < 0.0f)
            cubeIndex |= (1 << i);

    if (EDGE_TABLE[cubeIndex] == 0)
        return;

    // Corner positions
    glm::vec3 corners[8];
    const glm::vec3 sz = cell.size();
    for (int i = 0; i < 8; i++)
        corners[i] = cell.min + CORNER_OFFSETS[i] * sz;

    // Interpolate edge vertices
    glm::vec3 edgeVerts[12];
    for (int e = 0; e < 12; e++)
    {
        if (EDGE_TABLE[cubeIndex] & (1 << e))
        {
            const int a = EDGE_TO_CORNERS[e][0];
            const int b = EDGE_TO_CORNERS[e][1];
            edgeVerts[e] = Interp(corners[a], corners[b], values[a], values[b]);
        }
    }

    // Gradient step tied to cell size
    float h = 0.25f * std::min(sz.x, std::min(sz.y, sz.z));
    h = std::clamp(h, 1e-4f, 1e-1f);


	for (int i = 0; TRI_TABLE[cubeIndex][i] != -1; i += 3)
	{
		uint32_t ia = TRI_TABLE[cubeIndex][i + 0];
		uint32_t ib = TRI_TABLE[cubeIndex][i + 1];
		uint32_t ic = TRI_TABLE[cubeIndex][i + 2];

		glm::vec3 p0 = edgeVerts[ia];
		glm::vec3 p1 = edgeVerts[ib];
		glm::vec3 p2 = edgeVerts[ic];

		glm::vec3 faceN = glm::cross(p1 - p0, p2 - p0);
		const float faceLen2 = glm::dot(faceN, faceN);
		if (faceLen2 > 1e-20f) faceN *= (1.0f / std::sqrt(faceLen2));
		else faceN = glm::vec3(0,1,0);

		auto safeNorm = [&](glm::vec3 v) -> glm::vec3
		{
			float l2 = glm::dot(v, v);
			if (l2 < 1e-20f) return faceN;
			return v * (1.0f / std::sqrt(l2));
		};

		glm::vec3 n0 = safeNorm(GradientCentral(F, p0, h));
		glm::vec3 n1 = safeNorm(GradientCentral(F, p1, h));
		glm::vec3 n2 = safeNorm(GradientCentral(F, p2, h));

		const glm::vec3 c = (p0 + p1 + p2) * (1.0f / 3.0f);
		glm::vec3 gc = safeNorm(GradientCentral(F, c, h));

		if (glm::dot(faceN, gc) < 0.0f)
		{
			std::swap(p1, p2);
			std::swap(n1, n2);
		}

		const uint32_t v0 = GetOrCreateVertex(mesh, p0);
		const uint32_t v1 = GetOrCreateVertex(mesh, p1);
		const uint32_t v2 = GetOrCreateVertex(mesh, p2);

		AccumNormal(mesh, v0, n0);
		AccumNormal(mesh, v1, n1);
		AccumNormal(mesh, v2, n2);

		mesh.m_CPU.Indices.push_back(v0);
		mesh.m_CPU.Indices.push_back(v1);
		mesh.m_CPU.Indices.push_back(v2);
	}
}

static void LeafMicroMarch(
    const AABB& b,
    const std::function<float(float,float,float)>& eval,
    SurfaceMesh& mesh)
{
    const int SUB = 2;
    const glm::vec3 mn = b.min;
    const glm::vec3 mx = b.max;
    const glm::vec3 sz = (mx - mn) / float(SUB);

    for (int iz = 0; iz < SUB; ++iz)
    for (int iy = 0; iy < SUB; ++iy)
    for (int ix = 0; ix < SUB; ++ix)
    {
        glm::vec3 c0 = mn + glm::vec3(ix, iy, iz) * sz;
        glm::vec3 c1 = c0 + sz;

        AABB cb(c0, c1);

        glm::vec3 p[8] = {
            {c0.x, c0.y, c0.z},
            {c1.x, c0.y, c0.z},
            {c1.x, c1.y, c0.z},
            {c0.x, c1.y, c0.z},
            {c0.x, c0.y, c1.z},
            {c1.x, c0.y, c1.z},
            {c1.x, c1.y, c1.z},
            {c0.x, c1.y, c1.z}
        };

        float vals[8];
        uint8_t mask = 0;

        for (int i = 0; i < 8; ++i)
        {
            float v = eval(p[i].x, p[i].y, p[i].z);
            if (!std::isfinite(v)) v = 1e30f;
            vals[i] = v;
            if (v < 0.0f) mask |= (1u << i);
        }

        if (mask != 0x00 && mask != 0xFF)
            MarchingCubes(cb, vals, eval, mesh);
    }
}

static void ExtractImplicitMesh(
    OctreeNode* node,
    const std::function<float(float,float,float)>& eval,
    SurfaceMesh& mesh)
{
    if (!node) return;

    if (node->isLeaf)
    {
        const uint8_t signs = node->cornerSigns;

        if (signs != 0x00 && signs != 0xFF)
        {
            float vals[8];
            for (int i = 0; i < 8; ++i) vals[i] = node->cornerValues[i];
            MarchingCubes(node->bounds, vals, eval, mesh);
            return;
        }

        const glm::vec3 mn = node->bounds.min;
        const glm::vec3 mx = node->bounds.max;
        const glm::vec3 c  = 0.5f * (mn + mx);

        const bool cornerNeg = (signs == 0xFF);

        auto signDiffers = [&](float v) -> bool
        {
            bool neg = (v < 0.0f);
            return neg != cornerNeg;
        };

        bool maybeCrosses = false;

        {
            float vc = eval(c.x, c.y, c.z);
            if (!std::isfinite(vc)) vc = 1e30f;
            if (signDiffers(vc)) maybeCrosses = true;
        }

        if (!maybeCrosses)
        {
            glm::vec3 fc[6] = {
                {c.x, c.y, mn.z},
                {c.x, c.y, mx.z},
                {mn.x, c.y, c.z},
                {mx.x, c.y, c.z},
                {c.x, mn.y, c.z},
                {c.x, mx.y, c.z},
            };

            for (int i = 0; i < 6 && !maybeCrosses; ++i)
            {
                float v = eval(fc[i].x, fc[i].y, fc[i].z);
                if (!std::isfinite(v)) v = 1e30f;
                if (signDiffers(v)) maybeCrosses = true;
            }
        }

        if (!maybeCrosses)
        {
            glm::vec3 p[8] = {
                {mn.x, mn.y, mn.z},
                {mx.x, mn.y, mn.z},
                {mx.x, mx.y, mn.z},
                {mn.x, mx.y, mn.z},
                {mn.x, mn.y, mx.z},
                {mx.x, mn.y, mx.z},
                {mx.x, mx.y, mx.z},
                {mn.x, mx.y, mx.z}
            };

            const int edges[12][2] = {
                {0,1},{1,2},{2,3},{3,0},
                {4,5},{5,6},{6,7},{7,4},
                {0,4},{1,5},{2,6},{3,7}
            };

            for (int e = 0; e < 12 && !maybeCrosses; ++e)
            {
                glm::vec3 m = 0.5f * (p[edges[e][0]] + p[edges[e][1]]);
                float v = eval(m.x, m.y, m.z);
                if (!std::isfinite(v)) v = 1e30f;
                if (signDiffers(v)) maybeCrosses = true;
            }
        }

        if (maybeCrosses)
            LeafMicroMarch(node->bounds, eval, mesh);

        return;
    }

    for (int i = 0; i < 8; ++i)
        ExtractImplicitMesh(node->children[i], eval, mesh);
}

void SurfaceMesh::Build(const SurfaceSamplingConfig& cfg, const SurfaceEvaluator& eval, int samplingDepth)
{

	if (eval.IsEmpty())
		return;

	if (eval.SurfType == SurfaceType::ExplicitXY   ||
		eval.SurfType == SurfaceType::ExplicitX    ||
		eval.SurfType == SurfaceType::CompositionX ||
		eval.SurfType == SurfaceType::CompositionY)
	{
		BuildExplicit(cfg.s2, eval);
		m_Built = true;
		return;
	}

	if (eval.SurfType == SurfaceType::Implicit)
	{
		BuildImplicit(cfg.s3, eval, samplingDepth);
		m_Built = true;
		return;
	}

}

void SurfaceMesh::BuildExplicit(const SurfaceSampling& s, const SurfaceEvaluator& eval)
{


	m_CPU.Vertices.clear();

	m_CPU.Indices.clear();
	if (s.resolution < 2)
	{
		m_Built = false;
		return;
	}

	const int N = s.resolution;
	std::vector<bool> valid(N * N, false);
	std::vector<float> heights(N * N, 0.0f);


	float dx = (s.xMax - s.xMin) / (N - 1);
	float dy = (s.yMax - s.yMin) / (N - 1);

	for (int y = 0; y < N; ++y)
		for (int x = 0; x < N; ++x)
		{
			float Xw = s.xMin + x * dx;
			float Yw = s.yMin + y * dy;

			float k = s.contentScale; // domainSpan / boxSpan
			auto sample = eval.Eval(Xw * k, Yw * k);

			float Zw = (k != 0.0f) ? (float)(sample.z / k) : (float)sample.z;

			Vertex v{};
			v.Position = { Xw, Zw, Yw };
			v.Normal   = glm::vec3(0.0f, 1.0f, 0.0f);
			v.TexCoord = { x / float(N - 1), y / float(N - 1) };
			v.Color    = { 0.2f, 0.8f, 0.3f };

			int idx = y * N + x;

			m_CPU.Vertices.push_back(v);

			valid[idx] = sample.valid;
			heights[idx] = Zw;
		}

	// Build Indices
	for (int y = 0; y < N - 1; ++y)
	{
		for (int x = 0; x < N - 1; ++x)
		{
			uint32_t i0 =  y      * N + x;
			uint32_t i1 =  y      * N + (x + 1);
			uint32_t i2 = (y + 1) * N + x;
			uint32_t i3 = (y + 1) * N + (x + 1);

			if (!valid[i0] || !valid[i1] || !valid[i2] || !valid[i3])
				continue;

			// Only add triangles if all 3 vertices are valid
			if (valid[i0] && valid[i1] && valid[i2])
			{
				m_CPU.Indices.push_back(i0);
				m_CPU.Indices.push_back(i1);
				m_CPU.Indices.push_back(i2);
			}

			if (valid[i1] && valid[i3] && valid[i2])
			{
				m_CPU.Indices.push_back(i1);
				m_CPU.Indices.push_back(i3);
				m_CPU.Indices.push_back(i2);
			}
		}
	}

	auto H = [&](int yy, int xx) -> float {
		yy = std::clamp(yy, 0, N-1);
		xx = std::clamp(xx, 0, N-1);
		return heights[yy*N + xx];
	};

	auto V = [&](int yy, int xx) -> bool {
		yy = std::clamp(yy, 0, N-1);
		xx = std::clamp(xx, 0, N-1);
		return valid[yy*N + xx];
	};

	for (int y = 0; y < N; ++y)
		for (int x = 0; x < N; ++x)
		{
			const int idx = y*N + x;
			if (!valid[idx]) {
				m_CPU.Vertices[idx].Normal = glm::vec3(0,1,0);
				continue;
			}

			// Choose neighbor samples (fall back to one-sided differences if needed)
			int xl = x-1, xr = x+1, yd = y-1, yu = y+1;

			// If neighbors invalid, clamp them to current point (prevents wild normals)
			float hl = (xl >= 0 && V(y, xl)) ? H(y, xl) : H(y, x); // left  (x-1, z)
			float hr = (xr <  N && V(y, xr)) ? H(y, xr) : H(y, x); // right (x+1, z)

			float hd = (yd >= 0 && V(yd, x)) ? H(yd, x) : H(y, x); // down  (x, z-1)
			float hu = (yu <  N && V(yu, x)) ? H(yu, x) : H(y, x); // up    (x, z+1)

			float dfdx = (hr - hl) / ( ( (xr < N && xl >= 0) ? (2.0f*dx) : dx ) );
			float dfdz = (hu - hd) / ( ( (yu < N && yd >= 0) ? (2.0f*dy) : dy ) );

			glm::vec3 n(-dfdx, 1.0f, dfdz);

			// Ensure it points up
			if (n.y < 0.0f) n = -n;

			m_CPU.Vertices[idx].Normal = glm::normalize(n);
		}

}

void SurfaceMesh::BuildImplicit(const SurfaceSampling3D& s3, const SurfaceEvaluator& eval, const int& depth)
{
	m_ImplicitContentScale = s3.contentScale;

	Mesh newCPU{};
	AABB bounds(s3.min, s3.max);

	auto baseF = eval.GetCallableImplicit();

	const glm::vec3 sz = bounds.size();
	const float domainSize = std::max(sz.x, std::max(sz.y, sz.z));
	const int   N = std::max(4, s3.nx);

	float minCellSize = domainSize / float(N);

	int maxDepth = depth;
	maxDepth = std::clamp(maxDepth, 3, MAX_OCTREE_DEPTH);

	int forceLevels = 2;

	const float k = s3.contentScale;
	const float iso = s3.iso;

	auto signedF = [baseF, iso, k](float x, float y, float z) -> float
	{
		float v = 1e30f;
		try
		{
			v = baseF(x * k, y * k, z * k);
		}
		catch (...)
		{
			return 1e30f;
		}

		if (!std::isfinite(v)) return std::copysign(1e30f, v);

		v -= iso;
		if (!std::isfinite(v)) return std::copysign(1e30f, v);

		return v;
	};

	Octree tree(bounds, maxDepth, minCellSize, forceLevels);
	tree.Subdivide(tree.root, signedF, maxDepth);

	if (tree.Exhausted())
	{
		LOG("Octree pool exhausted!");
		return;
	}

	SurfaceMesh scratch;
	scratch.m_ImplicitContentScale = m_ImplicitContentScale;
	scratch.m_CPU = std::move(newCPU);

	scratch.m_VertexCache.clear();
	scratch.m_QuantizeStep = 0.0f; // disable quantized welding for now
	// scratch.m_QuantizeStep = std::max(minCellSize * 0.25f, 1e-4f);

	ExtractImplicitMesh(tree.root, signedF, scratch);
	FinalizeNormals(scratch);

	if (scratch.m_CPU.Indices.empty() || scratch.m_CPU.Vertices.empty())
		return;

	m_CPU = std::move(scratch.m_CPU);
	m_Built = true;
}


void SurfaceMesh::Draw() const
{
	if (!m_Built)
		return;

	m_GPU.Draw();
}

