#include "Shader.h"
#include <fstream>
#include <sstream>
#include "../utils/Log.h"


std::string Shader::LoadFile(const std::string &path)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		LOGError("Failed to load file: ", path);
		throw std::runtime_error("Shader file load failed");
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

unsigned int Shader::CompileShader(GLenum type, const std::string &source)
{
	// Compile shader
	unsigned int ID = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(ID, 1, &src, nullptr);
	glCompileShader(ID);

	// Handle errors
	int compileStatus;
	glGetShaderiv(ID, GL_COMPILE_STATUS, &compileStatus);
	if (compileStatus == GL_FALSE)
	{
		char infoLog[512];
		glGetShaderInfoLog(ID, 512, nullptr, infoLog);
		LOGError
		(
			"Shader compile error (",
			(type == GL_VERTEX_SHADER ? "VERTEX" : "Fragment"),
			"):\n", infoLog
		);
	}
	return ID;
}

Shader::Shader(const std::string &vertexPath, const std::string &fragmentPath)
{
	// Fetch shaders and compile
	std::string fullVertPath =  "../shaders/" + vertexPath;
	std::string fullFragPath =  "../shaders/" + fragmentPath;
	std::string vCode = LoadFile(fullVertPath);
	std::string fCode = LoadFile(fullFragPath);

	unsigned int vs = CompileShader(GL_VERTEX_SHADER, vCode);
	unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fCode);

	// Link into program
	ID = glCreateProgram();
	glAttachShader(ID, vs);
	glAttachShader(ID, fs);

	glLinkProgram(ID);

	// Handle errors
	int linkStatus;
	glGetProgramiv(ID, GL_LINK_STATUS, &linkStatus);
	if (linkStatus == GL_FALSE)
	{
		char infoLog[512];
		glGetProgramInfoLog(ID, 512, nullptr, infoLog);
		LOGError("ERROR::PROGRAM::LINKING_FAILED\n", infoLog);
		throw std::runtime_error("Shader(s) failed to link");
	}

	// Discard
	glDeleteShader(vs);
	glDeleteShader(fs);
}

Shader::~Shader() {};


/*
	TODO: Implement SetUniforms() and UnsetUniforms()
*/