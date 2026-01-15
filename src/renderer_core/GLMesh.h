#pragma once
#include <cstdint>
#include "Mesh.h"

class GLMesh
{
public:
	GLMesh();
	~GLMesh();

	void Upload(const Mesh& mesh);
	void Draw() const;

private:
	uint32_t m_VAO = 0;
	uint32_t m_VBO = 0;
	uint32_t m_EBO = 0;
	uint32_t m_IndexCount = 0;
};