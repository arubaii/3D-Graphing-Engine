#include "AssetManager.h"
#include "Loaders.h"

std::unordered_map<AssetHandle, AssetMetaData> AssetManager::s_Metadata{};
std::unordered_map<AssetHandle, Ref<Asset>>   AssetManager::s_LoadedAssets{};
std::unordered_map<AssetType, Scope<AssetLoader>> AssetManager::s_Serializers{};

void AssetManager::Init()
{

	s_Serializers[AssetType::Texture]  = CreateScope<TextureLoader>();
	s_Serializers[AssetType::Shader]   = CreateScope<ShaderLoader>();
	s_Serializers[AssetType::Material] = CreateScope<MaterialLoader>();
	s_Serializers[AssetType::Mesh]     = CreateScope<MeshLoader>();
	s_Serializers[AssetType::Model]    = CreateScope<ModelLoader>();
}

// Helper (internal linkage)
static AssetType DeduceAssetType(const std::filesystem::path& path)
{
	auto ext = path.extension().string();

	if (ext == ".png" || ext == ".jpg")
		return AssetType::Texture;
	if (ext == ".vert" || ext == ".frag")
		return AssetType::Shader;
	if (ext == ".obj")
		return AssetType::Model;

	return AssetType::None;
}

// AssetManager implementation
AssetHandle AssetManager::ImportAsset(const std::filesystem::path& path)
{
	AssetHandle handle;

	AssetMetaData meta;
	meta.Handle   = handle;
	meta.FilePath = path;
	meta.Type     = DeduceAssetType(path); // Add in symbol

	s_Metadata[handle] = meta;
	return handle;
}

Ref<Asset> AssetManager::LoadAssetInternal(const AssetMetaData& meta)
{
	auto loaderIt = s_Serializers.find(meta.Type);
	if (loaderIt == s_Serializers.end())
		throw std::runtime_error("No loader registered for asset type");

	// loaderIt is an iterator into the unordered_map.
	// Dereferencing it yields a std::pair<const AssetType, Scope<AssetLoader>>.
	// ->second accesses the value (the loader), and LoadAsset(meta) is dispatched virtually.
	return loaderIt->second->LoadAsset(meta);
}

bool AssetManager::IsLoaded(AssetHandle handle)
{
	return s_LoadedAssets.find(handle) != s_LoadedAssets.end();
}

void AssetManager::UnloadAsset(AssetHandle handle)
{
	s_LoadedAssets.erase(handle);
}

bool AssetManager::Exists(AssetHandle handle)
{
	return s_Metadata.find(handle) != s_Metadata.end();
}

