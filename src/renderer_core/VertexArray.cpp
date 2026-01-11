#include "GLcommon.h"
#include "VertexArray.h"


VertexArray::VertexArray()
{
	glGenVertexArrays(1, &m_RendererID);
	m_AttribIndex = 0;
}

VertexArray::~VertexArray()
{
	glDeleteVertexArrays(1, &m_RendererID);
}

void VertexArray::Bind() const
{
	glBindVertexArray(m_RendererID);
}

void VertexArray::AddBuffer(const VertexBuffer& vb, const VertexBufferLayout& layout)
{
	Bind();
	vb.Bind();

	GLint bound = 0;
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bound);
	assert(bound != 0 && "No VBO bound when setting vertex attrib pointers!");

	const auto& elements = layout.GetElements();
	uint32_t offset = 0;

	for (const auto& e: elements)
	{
		glEnableVertexAttribArray(m_AttribIndex);
		glVertexAttribPointer
		(
			m_AttribIndex,
			e.count,
			VertexBufferElement::ShaderDataTypeToOpenGL(e.type),
			e.normalized ? GL_TRUE : GL_FALSE,
			layout.GetStride(),
			(const void*)(std::uintptr_t)offset
		);

		offset += e.count * VertexBufferElement::GetSizeOfType(e.type);
		++m_AttribIndex;
	}

}