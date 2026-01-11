#pragma once
#include <string>
#include <filesystem>

struct ShaderSource
{
	std::string VertexSource;
	std::string FragmentSource;
};


class ShaderParser
{
public:
	// Parses a shader file and extracts vertex/fragment sources
	static ShaderSource Parse(const std::filesystem::path& path);
};
