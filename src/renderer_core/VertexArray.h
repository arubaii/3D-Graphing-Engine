#pragma once
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"

class VertexArray
{
private:
	uint32_t m_RendererID;
	uint32_t m_AttribIndex;
public:
	VertexArray();
	~VertexArray();

	VertexArray(const VertexArray&) = delete;
	VertexArray& operator=(const VertexArray&) = delete;

	VertexArray(VertexArray&& other) noexcept
		: m_RendererID(other.m_RendererID), m_AttribIndex(other.m_AttribIndex)
	{
		other.m_RendererID = 0;
		other.m_AttribIndex = 0;
	}

	VertexArray& operator=(VertexArray&& other) noexcept
	{
		if (this != &other)
		{
			if (m_RendererID)
				glDeleteVertexArrays(1, &m_RendererID);

			m_RendererID = other.m_RendererID;
			m_AttribIndex = other.m_AttribIndex;

			other.m_RendererID = 0;
			other.m_AttribIndex = 0;
		}
		return *this;
	}

	void Bind() const;
	void AddBuffer(const VertexBuffer& vb, const VertexBufferLayout& layout);
	void ResetAttribIndex() { m_AttribIndex = 0; }
	uint32_t GetID() { return m_RendererID; }
};
