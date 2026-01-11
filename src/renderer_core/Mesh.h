#pragma once
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include "IndexBuffer.h"
#include "VertexArray.h"
#include "VertexBufferLayout.h"


struct Vertex
{
	glm::vec3 Position;	// location = 0
	glm::vec3 Color;	// location = 1
};

struct Mesh
{
	std::vector<Vertex>	  Vertices;
	std::vector<uint32_t> Indices;
};

struct GPUMesh
{
	VertexArray VA;
	VertexBuffer VB;
	IndexBuffer IB;

	GPUMesh(VertexArray&& va, VertexBuffer&& vb, IndexBuffer&& ib)
	: VA(std::move(va)), VB(std::move(vb)), IB(std::move(ib)) {}
};

class MeshRendererCache
{
public:
	static GPUMesh& GetOrCreate(Mesh& mesh)
	{
		static std::unordered_map<const Mesh*, GPUMesh> cache;

		if (auto found = cache.find(&mesh); found != cache.end())
			return found->second;

		VertexArray va;
		va.Bind();

		VertexBuffer vb(
			static_cast<uint32_t>(mesh.Vertices.size() * sizeof(Vertex)),
			mesh.Vertices.data()
		);

		VertexBufferLayout layout;
		layout.Push<float>(3);
		layout.Push<float>(3);

		va.AddBuffer(vb, layout);
		GLint enabled0 = 0, enabled1 = 0;
		glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled0);
		glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled1);
		assert(enabled0 == GL_TRUE && enabled1 == GL_TRUE);
		// IMPORTANT: bind EBO WHILE VAO IS BOUND
		IndexBuffer ib(
			static_cast<uint32_t>(mesh.Indices.size()),
			mesh.Indices.data()
		);
		ib.Bind();

		auto [it, inserted] = cache.emplace(
			&mesh,
			GPUMesh{ std::move(va), std::move(vb), std::move(ib) }
		);

		return it->second;
	}

};