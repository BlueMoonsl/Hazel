#pragma once

namespace Hazel {

	using RendererID = uint32_t;

	// API类型
	enum class RendererAPIType
	{
		None,
		OpenGL
	};

	// 渲染信息
	struct RenderAPICapabilities
	{
		std::string Vendor;		// 供应商
		std::string Renderer;	// 显卡型号
		std::string Version;	// 渲染API版本

		int MaxSamples;			// 最大MSAA级别
		float MaxAnisotropy;	// 最大各向异性过滤级别
	};

	class RendererAPI
	{
	private:

	public:
		static void Init();
		static void Shutdown();

		static void Clear(float r, float g, float b, float a);
		static void SetClearColor(float r, float g, float b, float a);

		static void DrawIndexed(unsigned int count, bool depthTest = true); 

		static RenderAPICapabilities& GetCapabilities()
		{
			static RenderAPICapabilities capabilities;
			return capabilities;
		}

		static RendererAPIType Current() { return s_CurrentRendererAPI; }
	private:
		static RendererAPIType s_CurrentRendererAPI;
	};


}