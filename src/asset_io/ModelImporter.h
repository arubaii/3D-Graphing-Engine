#pragma once
#include <vector>
#include <filesystem>
#include "asset_core/Asset.h"

struct ModelImportData
{
	struct Submesh
	{
		AssetHandle Mesh;
		AssetHandle Material;
		uint32_t IndexOffset = 0;
		uint32_t IndexCount  = 0;
	};

	std::vector<Submesh> Submeshes;
};

class ModelImporter
{
public:
	static ModelImportData Import(const std::filesystem::path& path);
};
