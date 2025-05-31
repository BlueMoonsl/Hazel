#pragma once

#include "Hazel/Core/Base.h"

namespace Hazel {

	// 纹理格式
	enum class HAZEL_API TextureFormat
	{
		None = 0,
		RGB = 1,
		RGBA = 2,
	};

	class HAZEL_API Texture
	{
	public:
		virtual ~Texture() {}
	};

	class HAZEL_API Texture2D : public Texture
	{
	public:
		static Texture2D* Create(TextureFormat format, unsigned int width, unsigned int height);

		virtual TextureFormat GetFormat() const = 0;
		virtual unsigned int GetWidth() const = 0;
		virtual unsigned int GetHeight() const = 0;
	};

}