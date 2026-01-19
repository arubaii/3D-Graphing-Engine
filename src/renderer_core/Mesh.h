#pragma once
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include "IndexBuffer.h"
#include "VertexArray.h"
#include "VertexBufferLayout.h"


struct Vertex
{
	glm::vec3 Position;   // location = 0
	glm::vec3 Normal;     // location = 1
	glm::vec2 TexCoord;   // location = 2
	glm::vec3 Color;      // location = 3 (optional / debug)
};


struct Mesh
{
	std::vector<Vertex>	  Vertices;
	std::vector<uint32_t> Indices;

	uint64_t Revision = 0;
	void Touch() { ++Revision; }
};

struct LightVertex
{
	glm::vec3 Position;
};

struct LightMesh
{
	std::vector<LightVertex> Vertices;
	std::vector<uint32_t> Indices;
};


struct GPUMesh
{
	VertexArray VA;
	VertexBuffer VB;
	IndexBuffer IB;

	uint64_t LastRevision = 0;

	GPUMesh(VertexArray&& va, VertexBuffer&& vb, IndexBuffer&& ib)
	: VA(std::move(va)), VB(std::move(vb)), IB(std::move(ib)) {}
};

class MeshRendererCache
{
private:
	static std::unordered_map<const Mesh*, GPUMesh> s_Cache;
public:

	static void Invalidate(const Mesh& mesh);
	static GPUMesh& GetOrCreate(Mesh& mesh);

};