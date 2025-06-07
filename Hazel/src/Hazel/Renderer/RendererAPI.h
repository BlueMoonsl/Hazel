#pragma once

namespace Hazel {

	using RendererID = uint32_t;

	// API类型
	enum class RendererAPIType
	{
		None,
		OpenGL
	};

	// TODO: move into separate header
	enum class PrimitiveType
	{
		None = 0, Triangles, Lines
	};

	// 渲染信息
	struct RenderAPICapabilities
	{
		std::string Vendor;		// 供应商
		std::string Renderer;	// 显卡型号
		std::string Version;	// 渲染API版本

		int MaxSamples = 0;			// 最大MSAA级别
		float MaxAnisotropy = 0.0f;	// 最大各向异性过滤级别
		int MaxTextureUnits = 0;
	};

	class RendererAPI
	{
	private:

	public:
		static void Init();
		static void Shutdown();

		static void Clear(float r, float g, float b, float a);
		static void SetClearColor(float r, float g, float b, float a);

		static void DrawIndexed(uint32_t count, PrimitiveType type, bool depthTest = true);
		static void SetLineThickness(float thickness);

		static RenderAPICapabilities& GetCapabilities()
		{
			static RenderAPICapabilities capabilities;
			return capabilities;
		}

		static RendererAPIType Current() { return s_CurrentRendererAPI; }
	private:
		static void LoadRequiredAssets();
	private:
		static RendererAPIType s_CurrentRendererAPI;
	};


}