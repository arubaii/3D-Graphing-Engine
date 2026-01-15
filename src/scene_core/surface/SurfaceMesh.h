#pragma once
#include <array>
#include "SurfaceEvaluator.h"
#include "renderer_core/GLMesh.h"


struct SurfaceSampling
{
	float xMin, yMin;
	float xMax, yMax;
	int resolution;  // Numer of samples per axis; zoom out -> lower res
};

class SurfaceMesh
{
private:

	Mesh   m_CPU;
	GLMesh m_GPU;

	bool m_Built = false;

public:
	void Build(const SurfaceSampling& s, const SurfaceEvaluator& eval);
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