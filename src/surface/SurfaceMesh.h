#pragma once
#include <array>
#include "renderer_core/GLMesh.h"
#include "Octree.h"
#include "SurfaceEvaluator.h"


struct SurfaceSampling
{
	float xMin, yMin;
	float xMax, yMax;
	int resolution;  // Numer of samples per axis; zoom out -> lower res
	float contentScale = 1.0f;
};


struct SurfaceSampling3D
{
	glm::vec3 min;   // bounds in world
	glm::vec3 max;
	int nx, ny, nz;  // grid resolution
	float iso = 0.0f;
	float contentScale = 1.0f;

};

struct SurfaceSamplingConfig
{
	SurfaceSampling   s2;   // explicit
	SurfaceSampling3D s3;   // implicit
};



class SurfaceMesh
{
public:
	Mesh   m_CPU;
	GLMesh m_GPU;

	std::unordered_map<uint64_t, uint32_t> m_VertexCache;
	float m_QuantizeStep = 0.01f;
private:
	float m_ImplicitContentScale = 1.0f;
	bool m_Built = false;

public:
	void Build(const SurfaceSamplingConfig& cfg, const SurfaceEvaluator& eval, int samplingDepth);
	void BuildExplicit(const SurfaceSampling& s, const SurfaceEvaluator& eval);

	void BuildImplicit(const SurfaceSampling3D& s3, const SurfaceEvaluator& eval, const int& depth);

	void Draw() const;

	std::vector<Vertex>   GetVertices() const { return m_CPU.Vertices; }
	std::vector<uint32_t> GetIndices()  const { return m_CPU.Indices; }


};

