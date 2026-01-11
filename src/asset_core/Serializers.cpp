#include <yaml-cpp/yaml.h>
#include "Serializers.h"
#include "YamlTypes.h"

MaterialDesc MaterialSerializer::Deserialize(const std::filesystem::path& path)
{
	YAML::Node root = YAML::LoadFile(path.string());
	YAML::Node mat  = root["Material"];

	MaterialDesc desc;
	desc.ShaderProgram = mat["Shader"].as<AssetHandle>();
	desc.Albedo        = mat["Albedo"].as<AssetHandle>();

	auto color = mat["Color"];
	desc.Color = {
		color[0].as<float>(),
		color[1].as<float>(),
		color[2].as<float>()
	};

	return desc;
}

ShaderDesc ShaderSerializer::Deserialize(const std::filesystem::path& path)
{
	YAML::Node root = YAML::LoadFile(path.string());
	auto shader = root["Shader"];

	ShaderDesc desc;
	desc.VertexPath   = shader["Vertex"].as<std::string>();
	desc.FragmentPath = shader["Fragment"].as<std::string>();
	return desc;
}
