#pragma once
#include <filesystem>
#include <glm/vec3.hpp>
#include "Asset.h"
#include "AssetSerializer.h"


struct MaterialDesc // Short for Descriptor
{
	AssetHandle ShaderProgram;
	AssetHandle Albedo;
	glm::vec3   Color;
};

class MaterialSerializer : public AssetSerializer
{
public:
	static MaterialDesc Deserialize(const std::filesystem::path& path);
};

struct ShaderDesc
{
	std::filesystem::path VertexPath;
	std::filesystem::path FragmentPath;
};


class ShaderSerializer : public AssetSerializer
{
public:
	static ShaderDesc Deserialize(const std::filesystem::path& path);
};


