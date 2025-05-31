#include "hzpch.h"
#include "Texture.h"

#include "Hazel/Renderer/RendererAPI.h"
#include "Hazel/Platform/OpenGL/OpenGLTexture.h"

namespace Hazel {

	// 创建对应API的texture对象
	Texture2D* Texture2D::Create(TextureFormat format, unsigned int width, unsigned int height)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::OpenGL: return new OpenGLTexture2D(format, width, height);
		}
		return nullptr;
	}

}