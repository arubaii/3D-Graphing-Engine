#pragma once
#include <vector>
#include <cassert>
#include <cstdint>
#include "GLcommon.h"

enum class ShaderDataType : uint32_t
{
	Float,
	UInt,
	UByte
};

struct VertexBufferElement
{
	ShaderDataType type;
	uint32_t count;
	bool normalized;

	static uint32_t GetSizeOfType(ShaderDataType type)
	{
		switch (type)
		{
			case ShaderDataType::Float:		return 4;
			case ShaderDataType::UInt:		return 4;
			case ShaderDataType::UByte:		return 1;
		}
		assert(false && "Unknown type passed to GetSizeofType");
		return 0;
	}

	static GLenum ShaderDataTypeToOpenGL(ShaderDataType type)
	{
		switch (type)
		{
			case ShaderDataType::Float: return GL_FLOAT;
			case ShaderDataType::UInt:  return GL_UNSIGNED_INT;
			case ShaderDataType::UByte: return GL_UNSIGNED_BYTE;
		}
		throw std::runtime_error("Unknown type passed to ShaderDataType");
	}
};

class VertexBufferLayout
{
private:
	std::vector<VertexBufferElement> m_Elements;
	uint32_t m_Stride = 0;

public:
	template<typename T>
	void Push(uint32_t count)
	{
		static_assert(sizeof(T) == 0, "Unsupported type in VertexBufferLayout::Push");
	}

	const std::vector<VertexBufferElement>& GetElements() const { return m_Elements; }
	uint32_t GetStride() const { return m_Stride; }
};

template<>
inline void VertexBufferLayout::Push<float>(uint32_t count)
{
	m_Elements.push_back({ ShaderDataType::Float, static_cast<uint32_t>(count), GL_FALSE });
	m_Stride += VertexBufferElement::GetSizeOfType(ShaderDataType::Float) * count;
}

template<>
inline void VertexBufferLayout::Push<uint32_t>(uint32_t count)
{
	m_Elements.push_back({ ShaderDataType::UInt, static_cast<uint32_t>(count), GL_FALSE });
	m_Stride += VertexBufferElement::GetSizeOfType(ShaderDataType::UInt) * count;
}

template<>
inline void VertexBufferLayout::Push<std::uint8_t>(uint32_t count)
{
	m_Elements.push_back({ ShaderDataType::UByte, static_cast<uint32_t>(count), true });
	m_Stride += VertexBufferElement::GetSizeOfType(ShaderDataType::UByte) * count;
}

