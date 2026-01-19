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
	void Build(const SurfaceSamplingConfig& cfg, const SurfaceEvaluator& eval);
	void BuildExplicit(const SurfaceSampling& s, const SurfaceEvaluator& eval);

	void BuildImplicit(const SurfaceSampling3D& s3, const SurfaceEvaluator& eval);

	void Draw() const;

	std::vector<Vertex>   GetVertices() const { return m_CPU.Vertices; }
	std::vector<uint32_t> GetIndices()  const { return m_CPU.Indices; }


};




/*
	1.	Sample x and y over a square region x \in [xmin,xmax], y \in [ymin, ymax] (later dynamically sample for zoom size)
	2.	dx = (xmax - xmin)/(N_x - 1)
		dy = (ymax - ymin)/(N_y = 1)
	3.	For each grid point (i,j):
		x = xmin * i*dx
		y = ymin * j*dy
		z = f(x,y)
	4. Colorer heatmap based on height
	5. Index buffer:
		For each quad (i,j):

			(i,j+1)		(i+1,j+1)

			(i,j)		(i+1, j)

		a = idx(i, j)
		b = idx(i+1, j)
		c = idx(i, j+1)
		d = idx(i+1, j+1)

		tri1: a,b,c
		tri2: b,d,c

		standard draw call: glDrawElements(GL_TRIANGLES,...)

	6. Normals (worry about later)

	7. Implicit surfaces: x^2 + y^2 + z^2 = 1
		- Marching Cubes on a 3D grid of samples
		- extract triangle mesh
		- Draw call
*/