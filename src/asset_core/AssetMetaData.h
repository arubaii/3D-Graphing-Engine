#pragma once
#include <filesystem>
#include "AssetTypes.h"

struct AssetMetaData
{
	AssetHandle Handle;
	AssetType Type;
	std::filesystem::path FilePath;
	bool Loaded = false;

}; // Stored as std::unordered_map<AssetHandle, AssetMetaData> m_Registry;