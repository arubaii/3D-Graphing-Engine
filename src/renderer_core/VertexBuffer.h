#pragma once
#include <cstdint>

#include "VertexBufferLayout.h"


class VertexBuffer
{
private:
	uint32_t m_RendererID;

public:
	VertexBuffer() = default;
	VertexBuffer(uint32_t size, const void* data);
	~VertexBuffer();

	VertexBuffer(const VertexBuffer&) = delete;
	VertexBuffer& operator=(const VertexBuffer&) = delete;

	VertexBuffer(VertexBuffer&& other) noexcept
		: m_RendererID(other.m_RendererID)
	{
		other.m_RendererID = 0;
	}

	VertexBuffer& operator=(VertexBuffer&& other) noexcept
	{
		if (this != &other) {
			if (m_RendererID)
				glDeleteBuffers(1, &m_RendererID);
			m_RendererID = other.m_RendererID;
			other.m_RendererID = 0;
		}
		return *this;
	}


	uint32_t GetID() { return m_RendererID; }
	void Bind()   const;
	void Unbind() const; // Most likely unused
};
