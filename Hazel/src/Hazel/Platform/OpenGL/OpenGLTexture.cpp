#include "hzpch.h"
#include "OpenGLTexture.h"

#include "Hazel/Renderer/RendererAPI.h"
#include "Hazel/Renderer/Renderer.h"

#include <glad/glad.h>
#include "stb_image.h"

namespace Hazel {

	// 将Hazel的纹理格式枚举转换为OpenGL的纹理格式常量
	static GLenum HazelToOpenGLTextureFormat(TextureFormat format)
	{
		switch (format)
		{
			case Hazel::TextureFormat::RGB:     return GL_RGB;
			case Hazel::TextureFormat::RGBA:    return GL_RGBA;
		}
		return 0;
	}

	// 计算给定宽高下的最大MipMap等级数
	static int CalculateMipMapCount(int width, int height)
	{
		int levels = 1;
		while ((width | height) >> levels) {
			levels++;
		}
		return levels;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// Texture2D
	//////////////////////////////////////////////////////////////////////////////////

	// 构造函数：创建指定格式和尺寸的2D纹理
	OpenGLTexture2D::OpenGLTexture2D(TextureFormat format, unsigned int width, unsigned int height)
		: m_Format(format), m_Width(width), m_Height(height)
	{
		auto self = this;
		HZ_RENDER_1(self, {
			glGenTextures(1, &self->m_RendererID);
			glBindTexture(GL_TEXTURE_2D, self->m_RendererID);

			// 设置纹理参数
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTextureParameterf(self->m_RendererID, GL_TEXTURE_MAX_ANISOTROPY, RendererAPI::GetCapabilities().MaxAnisotropy);

			// 分配纹理存储空间
			glTexImage2D(GL_TEXTURE_2D, 0, HazelToOpenGLTextureFormat(self->m_Format), self->m_Width, self->m_Height, 0, HazelToOpenGLTextureFormat(self->m_Format), GL_UNSIGNED_BYTE, nullptr);
			glGenerateMipmap(GL_TEXTURE_2D);

			glBindTexture(GL_TEXTURE_2D, 0);
		});
	}

	// 构造函数：从文件加载2D纹理，支持sRGB格式
	OpenGLTexture2D::OpenGLTexture2D(const std::string& path, bool srgb)
		: m_FilePath(path)
	{
		int width, height, channels;
		HZ_CORE_INFO("Loading texture {0}, srgb={1}", path, srgb);
		m_ImageData = stbi_load(path.c_str(), &width, &height, &channels, srgb ? STBI_rgb : STBI_rgb_alpha);

		m_Width = width;
		m_Height = height;
		m_Format = TextureFormat::RGBA;

		HZ_RENDER_S1(srgb, {
			// sRGB纹理与普通纹理的创建方式略有不同
			if (srgb)
			{
				glCreateTextures(GL_TEXTURE_2D, 1, &self->m_RendererID);
				int levels = CalculateMipMapCount(self->m_Width, self->m_Height);
				HZ_CORE_INFO("Creating srgb texture width {0} mips", levels);
				glTextureStorage2D(self->m_RendererID, levels, GL_SRGB8, self->m_Width, self->m_Height);
				glTextureParameteri(self->m_RendererID, GL_TEXTURE_MIN_FILTER, levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
				glTextureParameteri(self->m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				glTextureSubImage2D(self->m_RendererID, 0, 0, 0, self->m_Width, self->m_Height, GL_RGB, GL_UNSIGNED_BYTE, self->m_ImageData);
				glGenerateTextureMipmap(self->m_RendererID);
			}
			else
			{
				glGenTextures(1, &self->m_RendererID);
				glBindTexture(GL_TEXTURE_2D, self->m_RendererID);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

				glTexImage2D(GL_TEXTURE_2D, 0, HazelToOpenGLTextureFormat(self->m_Format), self->m_Width, self->m_Height, 0, srgb ? GL_SRGB8 : HazelToOpenGLTextureFormat(self->m_Format), GL_UNSIGNED_BYTE, self->m_ImageData);
				glGenerateMipmap(GL_TEXTURE_2D);

				glBindTexture(GL_TEXTURE_2D, 0);
			}
			// 释放stb_image加载的图像数据
			stbi_image_free(self->m_ImageData);
			});
	}

	// 析构函数：释放OpenGL纹理资源
	OpenGLTexture2D::~OpenGLTexture2D()
	{
		auto self = this;
		HZ_RENDER_1(self, {
			glDeleteTextures(1, &self->m_RendererID);
		});
	}

	// 绑定2D纹理到指定纹理槽
	void OpenGLTexture2D::Bind(unsigned int slot) const
	{
		HZ_RENDER_S1(slot, {
			glBindTextureUnit(slot, self->m_RendererID);
			});
	}


	//////////////////////////////////////////////////////////////////////////////////
	// TextureCube
	//////////////////////////////////////////////////////////////////////////////////

	// 构造函数：从单张贴图加载立方体纹理（假设为3x4布局的cross贴图）
	OpenGLTextureCube::OpenGLTextureCube(const std::string& path)
		: m_FilePath(path)
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load(false);
		m_ImageData = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb);

		m_Width = width;
		m_Height = height;
		m_Format = TextureFormat::RGB;

		unsigned int faceWidth = m_Width / 4;
		unsigned int faceHeight = m_Height / 3;
		HZ_CORE_ASSERT(faceWidth == faceHeight, "Non-square faces!"); // 立方体每个面必须为正方形

		std::array<unsigned char*, 6> faces;
		for (size_t i = 0; i < faces.size(); i++)
			faces[i] = new unsigned char[faceWidth * faceHeight * 3]; // 3字节每像素

		int faceIndex = 0;

		// 提取水平方向的4个面
		for (size_t i = 0; i < 4; i++)
		{
			for (size_t y = 0; y < faceHeight; y++)
			{
				size_t yOffset = y + faceHeight;
				for (size_t x = 0; x < faceWidth; x++)
				{
					size_t xOffset = x + i * faceWidth;
					faces[faceIndex][(x + y * faceWidth) * 3 + 0] = m_ImageData[(xOffset + yOffset * m_Width) * 3 + 0];
					faces[faceIndex][(x + y * faceWidth) * 3 + 1] = m_ImageData[(xOffset + yOffset * m_Width) * 3 + 1];
					faces[faceIndex][(x + y * faceWidth) * 3 + 2] = m_ImageData[(xOffset + yOffset * m_Width) * 3 + 2];
				}
			}
			faceIndex++;
		}

		// 提取垂直方向的2个面（跳过中间的面）
		for (size_t i = 0; i < 3; i++)
		{
			// 跳过中间的面
			if (i == 1)
				continue;

			for (size_t y = 0; y < faceHeight; y++)
			{
				size_t yOffset = y + i * faceHeight;
				for (size_t x = 0; x < faceWidth; x++)
				{
					size_t xOffset = x + faceWidth;
					faces[faceIndex][(x + y * faceWidth) * 3 + 0] = m_ImageData[(xOffset + yOffset * m_Width) * 3 + 0];
					faces[faceIndex][(x + y * faceWidth) * 3 + 1] = m_ImageData[(xOffset + yOffset * m_Width) * 3 + 1];
					faces[faceIndex][(x + y * faceWidth) * 3 + 2] = m_ImageData[(xOffset + yOffset * m_Width) * 3 + 2];
				}
			}
			faceIndex++;
		}

		HZ_RENDER_S3(faces, faceWidth, faceHeight, {
			glGenTextures(1, &self->m_RendererID);
			glBindTexture(GL_TEXTURE_CUBE_MAP, self->m_RendererID);

			// 设置立方体纹理参数
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTextureParameterf(self->m_RendererID, GL_TEXTURE_MAX_ANISOTROPY, RendererAPI::GetCapabilities().MaxAnisotropy);

			auto format = HazelToOpenGLTextureFormat(self->m_Format);
			// 上传6个面的数据到OpenGL
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, format, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[2]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, format, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[0]);

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, format, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[4]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, format, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[5]);

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, format, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[1]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, format, faceWidth, faceHeight, 0, format, GL_UNSIGNED_BYTE, faces[3]);

			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

			glBindTexture(GL_TEXTURE_2D, 0);

			// 释放每个面的数据
			for (size_t i = 0; i < faces.size(); i++)
				delete[] faces[i];

			// 释放原始图像数据
			stbi_image_free(self->m_ImageData);
			});
	}

	// 析构函数：释放OpenGL立方体纹理资源
	OpenGLTextureCube::~OpenGLTextureCube()
	{
		auto self = this;
		HZ_RENDER_1(self, {
			glDeleteTextures(1, &self->m_RendererID);
			});
	}

	// 绑定立方体纹理到指定纹理槽
	void OpenGLTextureCube::Bind(unsigned int slot) const
	{
		HZ_RENDER_S1(slot, {
			glActiveTexture(GL_TEXTURE0 + slot);
			glBindTexture(GL_TEXTURE_CUBE_MAP, self->m_RendererID);
			});
	}
}