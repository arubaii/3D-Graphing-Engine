#include "SurfaceMesh.h"

void SurfaceMesh::Build(const SurfaceSampling& s, const SurfaceEvaluator& eval)
{
	m_CPU.Vertices.clear();
	m_CPU.Indices.clear();
	if (s.resolution < 2)
	{
		m_Built = false;
		return;
	}

	const int N = s.resolution;
	std::vector<bool> valid;
	valid.reserve(N * N);

	float dx = (s.xMax - s.xMin) / (N - 1);
	float dy = (s.yMax - s.yMin) / (N - 1);

	for (int y = 0; y < N; ++y)
		for (int x = 0; x < N; ++x)
		{

			float X = s.xMin + x*dx;
			float Y = s.yMin + y*dy;

			auto sample = eval.Eval(X, Y);

			Vertex v{};
			v.Position = { X, sample.z, Y }; // Opengl flips y and z axes
			v.Normal   = glm::vec3(0.0f, 1.0f, 0.0f); // TEMP
			v.TexCoord = { x / float(N - 1), y / float(N - 1) };
			v.Color    = {0.2f, 0.8f, 0.3f}; // Add heatmap options

			m_CPU.Vertices.push_back(v);
			valid.push_back(sample.valid);

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

			m_CPU.Indices.push_back(i0);
			m_CPU.Indices.push_back(i1);
			m_CPU.Indices.push_back(i2);

			m_CPU.Indices.push_back(i1);
			m_CPU.Indices.push_back(i3);
			m_CPU.Indices.push_back(i2);
		}
	}

	std::vector<glm::vec3> accumNormals(
	m_CPU.Vertices.size(),
	glm::vec3(0.0f)
);

	for (size_t i = 0; i < m_CPU.Indices.size(); i += 3)
	{
		uint32_t i0 = m_CPU.Indices[i + 0];
		uint32_t i1 = m_CPU.Indices[i + 1];
		uint32_t i2 = m_CPU.Indices[i + 2];

		const glm::vec3& p0 = m_CPU.Vertices[i0].Position;
		const glm::vec3& p1 = m_CPU.Vertices[i1].Position;
		const glm::vec3& p2 = m_CPU.Vertices[i2].Position;

		glm::vec3 n = glm::normalize(glm::cross(p1 - p0, p2 - p0));

		accumNormals[i0] += n;
		accumNormals[i1] += n;
		accumNormals[i2] += n;
	}

	for (size_t i = 0; i < m_CPU.Vertices.size(); ++i)
	{
		m_CPU.Vertices[i].Normal =
			glm::normalize(accumNormals[i]);
	}


	// Upload to GPU
	m_GPU.Upload(m_CPU);
	m_Built = true;
}



void SurfaceMesh::Draw() const
{
	if (!m_Built)
		return;

	m_GPU.Draw();
}

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