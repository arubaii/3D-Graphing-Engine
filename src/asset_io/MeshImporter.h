#pragma once
#include <vector>
#include <filesystem>

struct MeshData
{
	std::vector<float> Vertices;
	std::vector<uint32_t> Indices;
};

class MeshImporter
{
public:
	static MeshData Import(const std::filesystem::path& path);
};
