#include "hzpch.h"
#include "OpenGLTexture.h"

#include "Hazel/Renderer/RendererAPI.h"
#include "Hazel/Renderer/Renderer.h"

#include <Glad/glad.h>

namespace Hazel {

	static GLenum HazelToOpenGLTextureFormat(TextureFormat format)
	{
		switch (format)
		{
			case Hazel::TextureFormat::RGB:     return GL_RGB;
			case Hazel::TextureFormat::RGBA:    return GL_RGBA;
		}
		return 0;
	}

	OpenGLTexture2D::OpenGLTexture2D(TextureFormat format, unsigned int width, unsigned int height)
		: m_Format(format), m_Width(width), m_Height(height)
	{
		auto self = this;
		HZ_RENDER_1(self, {
			glGenTextures(1, &self->m_RendererID);
			glBindTexture(GL_TEXTURE_2D, self->m_RendererID);
			glTexImage2D(GL_TEXTURE_2D, 0, HazelToOpenGLTextureFormat(self->m_Format), self->m_Width, self->m_Height, 0, HazelToOpenGLTextureFormat(self->m_Format), GL_UNSIGNED_BYTE, nullptr);
			glBindTexture(GL_TEXTURE_2D, 0);
		});
	}

	OpenGLTexture2D::~OpenGLTexture2D()
	{
		auto self = this;
		HZ_RENDER_1(self, {
			glDeleteTextures(1, &self->m_RendererID);
		});
	}

}