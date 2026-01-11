#include "ShaderParser.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

ShaderSource ShaderParser::Parse(const std::filesystem::path& path)
{
	// TEMP STUB
	// Later: support #type vertex / #type fragment

	std::ifstream file(path);
	if (!file)
		throw std::runtime_error("ShaderParser: failed to open file");

	std::stringstream ss;
	ss << file.rdbuf();

	ShaderSource src;
	src.VertexSource   = ss.str();
	src.FragmentSource = ss.str();
	return src;
}
