#include <stdexcept>
#include "stb/stb_image.h" // Need
#include "AssetTypes.h"
#include "Loaders.h"
#include "AssetManager.h"
#include "Serializers.h"
#include "asset_io/MeshImporter.h"
#include "asset_io/ModelImporter.h"
#include "audio/AudioAPI.h"
#include "audio/AudioDecoder.h"
#include "renderer_core/Shader.h"

Ref<Asset> AudioClipLoader::LoadAsset(const AssetMetaData& meta)
{
	// Decode entire audio file into PCM
	AudioDecodeResult decoded = AudioDecoder::Decode(meta.FilePath);

	auto clip = CreateRef<AudioClipAsset>();
	clip->Handle     = meta.Handle;
	clip->Type       = meta.Type;
	clip->Channels   = decoded.Channels;
	clip->SampleRate = decoded.SampleRate;
	clip->Duration   = decoded.Duration;

	// Upload to audio backend
	clip->BufferID = AudioAPI::CreateBuffer(
		decoded.Samples,
		decoded.SampleCount,
		decoded.Channels,
		decoded.SampleRate
	);

	return clip;
}

Ref<Asset> AudioStreamLoader::LoadAsset(const AssetMetaData& meta)
{
	auto stream = CreateRef<AudioStreamAsset>();
	stream->Handle = meta.Handle;
	stream->Type   = meta.Type;

	// Create decoder only (no full decode)
	stream->StreamHandle = AudioDecoder::CreateStream(meta.FilePath);

	AudioStreamInfo info = AudioDecoder::GetStreamInfo(stream->StreamHandle);
	stream->Channels   = info.Channels;
	stream->SampleRate = info.SampleRate;
	stream->Duration   = info.Duration;

	return stream;
}

Ref<Asset> ShaderLoader::LoadAsset(const AssetMetaData& meta)
{

	// Shader is a render/runtime object created from two file paths
	// via Shader::Create(vertPath, fragPath).
	//
	// AssetMetaData  must tell paths.
	//
	// If meta.FilePath points to some "shader asset file" parse, parse it here to get:
	//   vertPath, fragPath

	ShaderDesc paths = ShaderSerializer::Deserialize(meta.FilePath);

	auto shaderAsset = CreateRef<ShaderAsset>();
	shaderAsset->Handle = meta.Handle;
	shaderAsset->Type   = meta.Type;

	shaderAsset->VertexSource   = paths.VertexPath;
	shaderAsset->FragmentSource = paths.FragmentPath;

	return shaderAsset;
}

Ref<Asset> MeshLoader::LoadAsset(const AssetMetaData& meta)
{
	// Repo has renderer_core::Mesh as a CPU mesh.
	// Doesn't use raw VAO/VBO/EBO IDs in a "Mesh" class.
	// So MeshAsset should eventually store that CPU mesh data (Vertices/Indices),
	// and the renderer uses MeshRendererCache::GetOrCreate(mesh) when drawing.

	MeshData meshData = MeshImporter::Import(meta.FilePath);

	auto mesh = CreateRef<MeshAsset>();
	mesh->Handle = meta.Handle;
	mesh->Type   = meta.Type;

	// If keeping VAO/VBO/EBO in MeshAsset: need GPU upload API.
	// If switching MeshAsset to store CPU vertices/indices: fill them here instead.
	mesh->IndexCount = static_cast<uint32_t>(meshData.Indices.size());

	return mesh;
}

Ref<Asset> MaterialLoader::LoadAsset(const AssetMetaData& meta)
{
	MaterialDesc desc = MaterialSerializer::Deserialize(meta.FilePath);

	auto material = CreateRef<MaterialAsset>();
	material->Handle = meta.Handle;
	material->Type   = meta.Type;

	material->ShaderProgram = AssetManager::GetAsset<ShaderAsset>(desc.ShaderProgram);
	material->Albedo        = AssetManager::GetAsset<TextureAsset>(desc.Albedo);
	material->Color         = desc.Color;

	return material;
}

Ref<Asset> ModelLoader::LoadAsset(const AssetMetaData& meta)
{
	ModelImportData data = ModelImporter::Import(meta.FilePath);

	auto model = CreateRef<ModelAsset>();
	model->Handle = meta.Handle;
	model->Type   = meta.Type;

	for (const auto& part : data.Submeshes)
	{
		ModelAsset::Submesh submesh;
		submesh.Mesh        = AssetManager::GetAsset<MeshAsset>(part.Mesh);
		submesh.Material    = AssetManager::GetAsset<MaterialAsset>(part.Material);
		submesh.IndexOffset = part.IndexOffset;
		submesh.IndexCount  = part.IndexCount;

		model->Submeshes.push_back(submesh);
	}

	return model;
}

Ref<Asset> TextureLoader::LoadAsset(const AssetMetaData& meta)
{
	int width, height, channels;
	stbi_uc* data = stbi_load(
		meta.FilePath.string().c_str(),
		&width, &height,
		&channels,
		4
	);

	if (!data)
		throw std::runtime_error("TextureLoader: failed to load image");

	auto texture = CreateRef<TextureAsset>();
	texture->Handle = meta.Handle;
	texture->Type   = meta.Type;
	texture->Width  = static_cast<uint32_t>(width);
	texture->Height = static_cast<uint32_t>(height);

	// Repo currently has no Texture class / GPU texture creation path,
	// RendererID must be created somewhere else (renderer) until  add it.
	// texture->RendererID = ...

	stbi_image_free(data);
	return texture;
}
