#pragma once
#include <memory>
#include <string>
#include "GLcommon.h"
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "scene_core/Camera/PerspectiveCamera.h"
#include "utils/SmartPtrs.h"


struct Shader
{
private:
	unsigned int ID;

	static std::string  LoadFile	  (const std::string &path);
	static unsigned int CompileShader (GLenum type, const std::string &s);
public:

	Shader(const std::string &vertexPath, const std::string &fragmentPath);
	~Shader();

	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;

	Shader(Shader&&) noexcept = default;
	Shader& operator=(Shader&&) noexcept = default;


	void Bind()   const { glUseProgram(ID); }
	void UnBind() const { glUseProgram(0); }

	unsigned int GetID() { return ID; }

	static Ref<Shader> Create(const std::string& vertPath, const std::string& fragPath)
	{
		return CreateRef<Shader>(vertPath, fragPath);
	}

	void SetBool  (const std::string& name, bool state) const
	{
		glUniform1f(glGetUniformLocation(ID, name.c_str()), (int)state);
	}
	void SetInt   (const std::string& name, int value) const
	{
		glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
	}
	void SetFloat (const std::string& name, float value) const
	{
		glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
	};
	void SetVec2	 (const std::string& name, float x, float y) const
	{
	    glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
	};
	void SetVec2	 (const std::string& name, const glm::vec2& v) const
	{
		glUniform2f(glGetUniformLocation(ID, name.c_str()), v.x, v.y);
	};
	void SetVec3	 (const std::string& name, float x, float y, float z) const
	{
	    glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
	};
	void SetVec3	 (const std::string& name, const glm::vec3& v) const
	{
		glUniform3f(glGetUniformLocation(ID, name.c_str()), v.x, v.y, v.z);
	};
	void SetVec4	 (const std::string& name, float x, float y, float z, float w) const
	{
		glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
	};
	void SetVec4	 (const std::string& name, const glm::vec4& v) const
	{
		glUniform4f(glGetUniformLocation(ID, name.c_str()), v.x, v.y, v.z, v.w);
	};
	void SetMat3	 (const std::string& name, const glm::mat3& M) const
	{
		glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(M));
	};
	void SetMat4	 (const std::string& name, const glm::mat4& M) const
	{
		glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(M));
	};

	void SetPhongUniforms
	(
		const glm::mat4& Model,
		const glm::mat4& Projection,
		const glm::mat4& lightModel,
		const glm::vec4& lightColor,
		PerspectiveCamera camera
	);

	void UnsetUniforms();


};
