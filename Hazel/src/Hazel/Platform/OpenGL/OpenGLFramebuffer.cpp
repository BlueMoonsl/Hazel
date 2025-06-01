#include "hzpch.h"
#include "OpenGLFramebuffer.h"

#include "Hazel/Renderer/Renderer.h"
#include <glad/glad.h>

namespace Hazel {

	OpenGLFramebuffer::OpenGLFramebuffer(uint32_t width, uint32_t height, FramebufferFormat format)
		: m_Width(width), m_Height(height), m_Format(format)
	{
		Resize(width, height);
	}

	OpenGLFramebuffer::~OpenGLFramebuffer()
	{
		HZ_RENDER_S({
			glDeleteFramebuffers(1, &self->m_RendererID); // 删除帧缓冲对象
			});
	}

	// 调整帧缓冲区大小，并重新分配相关OpenGL资源
	void OpenGLFramebuffer::Resize(uint32_t width, uint32_t height)
	{
		if (m_Width == width && m_Height == height)
			return;

		m_Width = width;
		m_Height = height;
		HZ_RENDER_S({
			// 如果已存在资源，先释放
			if (self->m_RendererID)
			{
				glDeleteFramebuffers(1, &self->m_RendererID);
				glDeleteTextures(1, &self->m_ColorAttachment);
				glDeleteTextures(1, &self->m_DepthAttachment);
			}

			// 创建新的帧缓冲对象
			glGenFramebuffers(1, &self->m_RendererID);
			glBindFramebuffer(GL_FRAMEBUFFER, self->m_RendererID);

			// 创建颜色附件纹理
			glGenTextures(1, &self->m_ColorAttachment);
			glBindTexture(GL_TEXTURE_2D, self->m_ColorAttachment);

			// 根据格式分配纹理存储
			// TODO: 后续可根据format创建Hazel自定义纹理对象
			if (self->m_Format == FramebufferFormat::RGBA16F)
			{
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, self->m_Width, self->m_Height, 0, GL_RGBA, GL_FLOAT, nullptr);
			}
			else if (self->m_Format == FramebufferFormat::RGBA8)
			{
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, self->m_Width, self->m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			}
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, self->m_ColorAttachment, 0);

			// 创建深度模板附件纹理
			glGenTextures(1, &self->m_DepthAttachment);
			glBindTexture(GL_TEXTURE_2D, self->m_DepthAttachment);
			glTexImage2D(
				GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, self->m_Width, self->m_Height, 0,
				GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL
			);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, self->m_DepthAttachment, 0);

			// 检查帧缓冲完整性
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				HZ_CORE_ERROR("Framebuffer is incomplete!");

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			});
	}

	void OpenGLFramebuffer::Bind() const
	{
		HZ_RENDER_S({
			glBindFramebuffer(GL_FRAMEBUFFER, self->m_RendererID);
			glViewport(0, 0, self->m_Width, self->m_Height);
			});
	}

	void OpenGLFramebuffer::Unbind() const
	{
		HZ_RENDER_S({
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			});
	}

	// 绑定颜色附件纹理到指定纹理槽
	void OpenGLFramebuffer::BindTexture(uint32_t slot) const
	{
		HZ_RENDER_S1(slot, {
			glActiveTexture(GL_TEXTURE0 + slot);
			glBindTexture(GL_TEXTURE_2D, self->m_ColorAttachment);
			});
	}
}