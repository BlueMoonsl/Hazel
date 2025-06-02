#include "hzpch.h"
#include "Renderer.h"

#include "Shader.h"

namespace Hazel {

	Renderer* Renderer::s_Instance = new Renderer();
	RendererAPIType RendererAPI::s_CurrentRendererAPI = RendererAPIType::OpenGL;
	
	void Renderer::Init()
	{
		s_Instance->m_ShaderLibrary = std::make_unique<ShaderLibrary>();
		HZ_RENDER({ RendererAPI::Init(); });

		Renderer::GetShaderLibrary()->Load("assets/shaders/HazelPBR_Static.glsl");
		Renderer::GetShaderLibrary()->Load("assets/shaders/HazelPBR_Anim.glsl");
	}

	void Renderer::Clear()
	{
		HZ_RENDER({
			RendererAPI::Clear(0.0f, 0.0f, 0.0f, 1.0f);
			});
	}

	void Renderer::Clear(float r, float g, float b, float a)
	{
		HZ_RENDER_4(r, g, b, a, {
			RendererAPI::Clear(r, g, b, a);
		});
	}

	void Renderer::ClearMagenta()
	{
		Clear(1, 0, 1);
	}

	void Renderer::SetClearColor(float r, float g, float b, float a)
	{
	}

	void Renderer::DrawIndexed(uint32_t count, bool depthTest)
	{
		HZ_RENDER_2(count, depthTest, {
			RendererAPI::DrawIndexed(count, depthTest);
		});
	}

	void Renderer::WaitAndRender()
	{
		s_Instance->m_CommandQueue.Execute();
	}

}