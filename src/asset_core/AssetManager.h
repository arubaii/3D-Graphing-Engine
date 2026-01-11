#pragma once
#include <utility>
#include "AssetMetaData.h"
#include "AssetLoader.h"


class AssetManager
{
private:
	static std::unordered_map<AssetHandle, AssetMetaData>	   s_Metadata;
	static std::unordered_map<AssetHandle, Ref<Asset>>		   s_LoadedAssets;
	static std::unordered_map<AssetType, Scope<AssetLoader>>   s_Serializers;

public:
	static void Init();

	static AssetHandle ImportAsset(const std::filesystem::path& path);

	template<typename T>
	static Ref<T> GetAsset(AssetHandle handle);
	static Ref<Asset> LoadAssetInternal(const AssetMetaData& meta);
	static void UnloadAsset(AssetHandle handle);

	static bool IsLoaded(AssetHandle handle);
	static bool Exists(AssetHandle handle);

};

template<typename T>
Ref<T> AssetManager::GetAsset(AssetHandle handle)
{
	static_assert(std::is_base_of_v<Asset, T>);

	auto it = s_LoadedAssets.find(handle);
	if (it != s_LoadedAssets.end())
		return std::static_pointer_cast<T>(it->second);

	const auto& meta = s_Metadata.at(handle);
	Ref<Asset> asset = LoadAssetInternal(meta);

	s_LoadedAssets[handle] = asset;
	return std::static_pointer_cast<T>(asset);
}
