#pragma once

#include "Hazel/Renderer/RendererAPI.h"
#include "Hazel/Renderer/Texture.h"

namespace Hazel {

	// OpenGL2D纹理
	class OpenGLTexture2D : public Texture2D
	{
	public:
		OpenGLTexture2D(TextureFormat format, unsigned int width, unsigned int height);
		OpenGLTexture2D(const std::string& path, bool srgb);
			virtual ~OpenGLTexture2D();

		virtual void Bind(unsigned int slot = 0) const;

		virtual TextureFormat GetFormat() const { return m_Format; }
		virtual unsigned int GetWidth() const { return m_Width; }
		virtual unsigned int GetHeight() const { return m_Height; }

		virtual const std::string& GetPath() const override { return m_FilePath; }

		virtual RendererID GetRendererID() const override { return m_RendererID; }
	private:
		RendererID m_RendererID;
		TextureFormat m_Format;
		unsigned int m_Width, m_Height;

		unsigned char* m_ImageData;

		std::string m_FilePath;
	};

	// OpenGL立方体纹理
	class OpenGLTextureCube : public TextureCube
	{
	public:
		OpenGLTextureCube(const std::string& path);
		  virtual ~OpenGLTextureCube();

		  virtual void Bind(unsigned int slot = 0) const;

		virtual TextureFormat GetFormat() const { return m_Format; }
		virtual unsigned int GetWidth() const { return m_Width; }
		virtual unsigned int GetHeight() const { return m_Height; }

		virtual const std::string& GetPath() const override { return m_FilePath; }

		virtual RendererID GetRendererID() const override { return m_RendererID; }
	private:
		RendererID m_RendererID;
		TextureFormat m_Format;
		unsigned int m_Width, m_Height;

		unsigned char* m_ImageData;

		std::string m_FilePath;
	};
}