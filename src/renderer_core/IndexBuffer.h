#pragma once
#include <cstdint>
#include "GLcommon.h"



class IndexBuffer
{
private:
	uint32_t m_RendererID;
	uint32_t m_Count;

public:
	IndexBuffer(uint32_t count, const unsigned int* data);
	~IndexBuffer();

	IndexBuffer(const IndexBuffer&) = delete;
	IndexBuffer& operator=(const IndexBuffer&) = delete;

	IndexBuffer(IndexBuffer&& other) noexcept
		: m_RendererID(other.m_RendererID), m_Count(other.m_Count)
	{
		other.m_RendererID = 0;
		other.m_Count = 0;
	}

	IndexBuffer& operator=(IndexBuffer&& other) noexcept
	{
		if (this != &other)
		{
			if (m_RendererID)
				glDeleteBuffers(1, &m_RendererID);

			m_RendererID = other.m_RendererID;
			m_Count = other.m_Count;

			other.m_RendererID = 0;
			other.m_Count = 0;
		}
		return *this;
	}
	void Bind() const;

	inline uint32_t GetCount() const { return m_Count; }
};