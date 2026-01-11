#pragma once
#include <glm/vec3.hpp>
#include "Asset.h"
#include "utils/SmartPtrs.h"
#include "renderer_core/Shader.h"

struct TextureAsset;


// <30 seconds
struct AudioClipAsset : public Asset
{
	uint32_t BufferID = 0;
	uint32_t Channels = 0;
	uint32_t SampleRate = 0;
	float Duration = 0.0f;
};

// >=30 seconds
struct AudioStreamAsset : public Asset
{
	// Opaque streaming source (decoder, handle, etc.)
	void* StreamHandle = nullptr;

	uint32_t Channels = 0;
	uint32_t SampleRate = 0;
	float Duration = 0.0f;
};

struct ShaderAsset : public Asset
{
	std::string VertexSource;
	std::string FragmentSource;
};


struct MaterialAsset : public Asset
{
	Ref<ShaderAsset>  ShaderProgram;
	Ref<TextureAsset> Albedo;
	glm::vec3    Color = {1.0f, 1.0f, 1.0f};
};

struct MeshAsset : public Asset
{
	uint32_t VAO = 0;
	uint32_t VBO = 0;
	uint32_t EBO = 0;
	uint32_t IndexCount = 0;
};

struct ModelAsset : public Asset
{
	struct Submesh
	{
		Ref<MeshAsset>     Mesh;
		Ref<MaterialAsset> Material;
		uint32_t      IndexOffset = 0;
		uint32_t      IndexCount  = 0;
	};

	std::vector<Submesh> Submeshes;
};


struct TextureAsset : public Asset
{
	uint32_t RendererID = 0;
	uint32_t Width = 0, Height = 0;
};


