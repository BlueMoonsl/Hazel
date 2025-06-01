#include "hzpch.h"
#include "Shader.h"

#include "Hazel/Platform/OpenGL/OpenGLShader.h"

namespace Hazel {

	std::vector<Shader*> Shader::s_AllShaders;

	// 创建不同API的shader
	Shader* Shader::Create(const std::string& filepath)
	{
		Shader* result = nullptr;

		switch (RendererAPI::Current())
		{
			case RendererAPIType::None: return nullptr;
			case RendererAPIType::OpenGL: result = new OpenGLShader(filepath);
		}
		s_AllShaders.push_back(result);
		return result;
	}

}