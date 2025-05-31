#include "hzpch.h"
#include "Shader.h"

#include "Hazel/Platform/OpenGL/OpenGLShader.h"

namespace Hazel {

	// 创建不同API的shader
	Shader* Shader::Create(const std::string& filepath)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::OpenGL: return new OpenGLShader(filepath);
		}
		return nullptr;
	}

}