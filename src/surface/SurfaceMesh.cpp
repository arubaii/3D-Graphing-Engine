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
	const float step = std::max(mesh.m_QuantizeStep, 1e-6f);
	const uint64_t key = QuantKey(pos, step);

	auto it = mesh.m_VertexCache.find(key);
	if (it != mesh.m_VertexCache.end())
		return it->second;

	Vertex v{};
	v.Position = pos;
	v.Normal   = glm::vec3(0.0f); // accumulate face normals; normalize later

	const uint32_t idx = (uint32_t)mesh.m_CPU.Vertices.size();
	mesh.m_CPU.Vertices.push_back(v);
	mesh.m_VertexCache.emplace(key, idx);
	return idx;
}

static inline void FinalizeNormals(SurfaceMesh& mesh)
{
	for (auto& v : mesh.m_CPU.Vertices)
	{
		const float len2 = glm::dot(v.Normal, v.Normal);
		if (len2 > 1e-20f)
			v.Normal = glm::normalize(v.Normal);
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

        // Per-vertex normals from gradient of signed field
        glm::vec3 n0 = GradientCentral(F, p0, h);
        glm::vec3 n1 = GradientCentral(F, p1, h);
        glm::vec3 n2 = GradientCentral(F, p2, h);

        // Normalize safely (fallback to face normal)
        glm::vec3 faceN = glm::cross(p1 - p0, p2 - p0);
        const float faceLen2 = glm::dot(faceN, faceN);
        if (faceLen2 > 1e-20f) faceN = faceN * (1.0f / std::sqrt(faceLen2));
        else faceN = glm::vec3(0,1,0);

        auto safeNorm = [&](glm::vec3 v) -> glm::vec3
        {
            float l2 = glm::dot(v, v);
            if (l2 < 1e-20f) return faceN;
            return v * (1.0f / std::sqrt(l2));
        };

        n0 = safeNorm(n0);
        n1 = safeNorm(n1);
        n2 = safeNorm(n2);

        // Ensure triangle winding matches "outward" direction.
        // Outward for signedF (v-iso) is along +grad(F) from inside->outside.
        // If face normal points opposite average grad, flip winding.
        const glm::vec3 c = (p0 + p1 + p2) * (1.0f / 3.0f);
        glm::vec3 gc = safeNorm(GradientCentral(F, c, h));

        if (glm::dot(faceN, gc) < 0.0f)
        {
            std::swap(p1, p2);
            std::swap(n1, n2);
            faceN = -faceN;
        }

        Vertex v0{}, v1{}, v2{};
        v0.Position = p0; v0.Normal = n0;
        v1.Position = p1; v1.Normal = n1;
        v2.Position = p2; v2.Normal = n2;

        const uint32_t base = (uint32_t)mesh.m_CPU.Vertices.size();
        mesh.m_CPU.Vertices.push_back(v0);
        mesh.m_CPU.Vertices.push_back(v1);
        mesh.m_CPU.Vertices.push_back(v2);

        mesh.m_CPU.Indices.push_back(base + 0);
        mesh.m_CPU.Indices.push_back(base + 1);
        mesh.m_CPU.Indices.push_back(base + 2);
    }
}


static void ExtractImplicitMesh(
	OctreeNode* node,
	const std::function<float(float,float,float)>& f,
	SurfaceMesh& mesh)
{
	if (!node) return;

	if (node->isLeaf)
	{
		const uint8_t signs = node->cornerSigns;

		if (signs != 0x00 && signs != 0xFF)
			MarchingCubes(node->bounds, node->cornerValues, f, mesh);
		return;
	}

	for (int i = 0; i < 8; ++i)
		ExtractImplicitMesh(node->children[i], f, mesh);
}


void SurfaceMesh::Build(const SurfaceSamplingConfig& cfg, const SurfaceEvaluator& eval)
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
		BuildImplicit(cfg.s3, eval);
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

void SurfaceMesh::BuildImplicit(const SurfaceSampling3D& s3, const SurfaceEvaluator& eval)
{
	m_ImplicitContentScale = s3.contentScale;

	Mesh newCPU{};
	AABB bounds(s3.min, s3.max);

	auto baseF = eval.GetCallableImplicit();

	const float domainSize = (bounds.max.x - bounds.min.x);
	const int   N = std::max(4, s3.nx);

	float minCellSize = domainSize / float(N);
	minCellSize = std::clamp(minCellSize, 0.01f, 10.0f);

	int maxDepth = (int)std::ceil(std::log2(std::max(domainSize / std::max(minCellSize, 1e-6f), 1.0f)));
	maxDepth = std::clamp(maxDepth, 3, 10);

	const float k   = s3.contentScale;
	const float iso = s3.iso;

	auto signedF = [baseF, iso, k](float x, float y, float z) -> float
	{
		float v = baseF(x * k, y * k, z * k);
		if (!std::isfinite(v)) return 1e30f;
		return v - iso;
	};

	const int forceLevels = std::clamp(2, 0, maxDepth);

	Octree tree(bounds, maxDepth, minCellSize, forceLevels);
	tree.Subdivide(tree.root, signedF, maxDepth);

	if (tree.Exhausted())
	{
		LOG("Octree pool exhausted!");
		LOG("implicit verts/idx: ", 0, " / ", 0);
		return;
	}

	SurfaceMesh scratch;
	scratch.m_ImplicitContentScale = m_ImplicitContentScale;
	scratch.m_CPU = std::move(newCPU);

	scratch.m_VertexCache.clear();
	scratch.m_QuantizeStep = std::max(minCellSize * 0.25f, 1e-4f);

	ExtractImplicitMesh(tree.root, signedF, scratch);

	if (scratch.m_CPU.Indices.empty() || scratch.m_CPU.Vertices.empty())
	{
		LOG("implicit verts/idx: ", 0, " / ", 0);
		return;
	}

	for (auto& v : scratch.m_CPU.Vertices)
		v.Normal = glm::vec3(0.0f);

	const auto& idx = scratch.m_CPU.Indices;
	auto& verts = scratch.m_CPU.Vertices;

	for (size_t i = 0; i + 2 < idx.size(); i += 3)
	{
		const uint32_t i0 = idx[i + 0];
		const uint32_t i1 = idx[i + 1];
		const uint32_t i2 = idx[i + 2];

		const glm::vec3 p0 = verts[i0].Position;
		const glm::vec3 p1 = verts[i1].Position;
		const glm::vec3 p2 = verts[i2].Position;

		glm::vec3 n = glm::cross(p1 - p0, p2 - p0);
		float len2 = glm::dot(n, n);
		if (len2 > 1e-20f)
			n *= (1.0f / std::sqrt(len2));
		else
			n = glm::vec3(0.0f, 1.0f, 0.0f);

		verts[i0].Normal += n;
		verts[i1].Normal += n;
		verts[i2].Normal += n;
	}

	for (auto& v : verts)
	{
		float len2 = glm::dot(v.Normal, v.Normal);
		if (len2 > 1e-20f)
			v.Normal *= (1.0f / std::sqrt(len2));
		else
			v.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
	}

	m_CPU = std::move(scratch.m_CPU);

	LOG("implicit verts/idx: ", (int)m_CPU.Vertices.size(), " / ", (int)m_CPU.Indices.size());

	m_Built = true;
}



void SurfaceMesh::Draw() const
{
	if (!m_Built)
		return;

	m_GPU.Draw();
}

