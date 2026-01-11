#pragma once
#include "renderer_core/VertexArray.h"
#include "renderer_core/IndexBuffer.h"
#include "renderer_core/Shader.h"


class Renderer
{
public:
	void SetShader(const std::shared_ptr<Shader>& shader)
    {
         m_Shader = shader;
    }

    void SetMVP(const glm::mat4& mvp)
    {
        assert(m_Shader);
        m_Shader->SetMat4("u_MVP", mvp);
    }
	void Clear() const;
	void Draw(const VertexArray& va, const IndexBuffer& ib) const;
private:
	std::shared_ptr<Shader> m_Shader;
};