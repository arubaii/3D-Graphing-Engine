#pragma once
#include "utils/UUID.h"

enum class AssetType : uint8_t
{
	None = 0,

	Texture,
	Mesh,
	Model,
	Shader,
	Material,
	Scene,
	Audio,
	Font
};

constexpr const char* AssetTypeToString(AssetType type)
{
	switch (type)
	{
		case AssetType::None:		return "None";
		case AssetType::Texture:	return "Texture";
		case AssetType::Mesh:		return "Mesh";
		case AssetType::Model:		return "Model";
		case AssetType::Shader:		return "Shader";
		case AssetType::Material:	return "Material";
		case AssetType::Scene:		return "Scene";
		case AssetType::Audio:		return "Audio";
		case AssetType::Font:		return "Font";
	}
}


using AssetHandle = UUID;

class Asset
{
public:
	AssetHandle Handle;
	AssetType   Type = AssetType::None;
	virtual ~Asset() = default;
};