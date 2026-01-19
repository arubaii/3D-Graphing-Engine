#include "Mesh.h"

std::unordered_map<const Mesh*, GPUMesh> MeshRendererCache::s_Cache;


void MeshRendererCache::Invalidate(const Mesh& mesh)
{
	auto it = s_Cache.find(&mesh);
	if (it != s_Cache.end())
		s_Cache.erase(it);
}

GPUMesh& MeshRendererCache::GetOrCreate(Mesh& mesh)
{
	auto it = s_Cache.find(&mesh);
	if (it == s_Cache.end())
	{
		VertexArray va;
		va.Bind();

		VertexBuffer vb(
			mesh.Vertices.size() * sizeof(Vertex),
			mesh.Vertices.data()
		);

		VertexBufferLayout layout;
		layout.Push<float>(3);
		layout.Push<float>(3);
		layout.Push<float>(2);
		layout.Push<float>(3);

		va.AddBuffer(vb, layout);

		IndexBuffer ib(
			mesh.Indices.size(),
			mesh.Indices.data()
		);

		ib.Bind();

		auto [insertedIt, ok] =
			s_Cache.emplace(&mesh,
							GPUMesh{
								std::move(va),
								std::move(vb),
								std::move(ib)
							});

		insertedIt->second.LastRevision = mesh.Revision;
		return insertedIt->second;
	}


	GPUMesh& gpu = it->second;


	if (gpu.LastRevision != mesh.Revision)
	{
		gpu.VA.Bind();

		gpu.VB.Bind();
		glBufferData(GL_ARRAY_BUFFER,
					 mesh.Vertices.size() * sizeof(Vertex),
					 mesh.Vertices.data(),
					 GL_DYNAMIC_DRAW);

		gpu.IB.Bind();
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
					 mesh.Indices.size() * sizeof(uint32_t),
					 mesh.Indices.data(),
					 GL_DYNAMIC_DRAW);

		gpu.LastRevision = mesh.Revision;
	}

	return gpu;
}
