#include "hzpch.h"
#include "Hazel/Renderer/RendererAPI.h"

#include <Glad/glad.h>

namespace Hazel {

	// OpenGL 调试信息回调函数
	static void OpenGLLogMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
	{
		if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
		{
			HZ_CORE_ERROR("{0}", message);      // 输出错误信息
			HZ_CORE_ASSERT(false, "");          // 断言失败
		}
		else
		{
			// HZ_CORE_TRACE("{0}", message);      // 输出普通调试信息
		}
	}

	// 初始化 OpenGL 渲染器
	void RendererAPI::Init()
	{
		glDebugMessageCallback(OpenGLLogMessage, nullptr); // 注册 OpenGL 调试回调
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);             // 同步输出调试信息

		unsigned int vao;
		glGenVertexArrays(1, &vao);        // 创建并绑定默认 VAO
		glBindVertexArray(vao);

		glEnable(GL_DEPTH_TEST);           // 启用深度测试
		//glEnable(GL_CULL_FACE);          // 可选：启用面剔除
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS); // 启用无缝立方体贴图
		glFrontFace(GL_CCW);               // 设置逆时针为正面

		glEnable(GL_BLEND);                // 启用混合
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // 设置混合函数

		auto& caps = RendererAPI::GetCapabilities();

		// 获取并保存显卡厂商、渲染器、OpenGL 版本等信息
		caps.Vendor = (const char*)glGetString(GL_VENDOR);
		caps.Renderer = (const char*)glGetString(GL_RENDERER);
		caps.Version = (const char*)glGetString(GL_VERSION);

		glGetIntegerv(GL_MAX_SAMPLES, &caps.MaxSamples); // 最大多重采样数
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &caps.MaxAnisotropy); // 最大各向异性过滤

		// 检查并输出所有 OpenGL 错误
		GLenum error = glGetError();
		while (error != GL_NO_ERROR)
		{
			HZ_CORE_ERROR("OpenGL Error {0}", error);
			error = glGetError();
		}

		LoadRequiredAssets();
	}

	// 关闭 OpenGL 渲染器（目前无操作）
	void RendererAPI::Shutdown()
	{
	}

	void RendererAPI::LoadRequiredAssets()
	{
	}

	// 清除颜色和深度缓冲区
	void RendererAPI::Clear(float r, float g, float b, float a)
	{
		glClearColor(r, g, b, a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	// 设置清屏颜色
	void RendererAPI::SetClearColor(float r, float g, float b, float a)
	{
		glClearColor(r, g, b, a);
	}

	// 绘制索引三角形，支持可选的深度测试开关
	void RendererAPI::DrawIndexed(unsigned int count, bool depthTest)
	{
		if (!depthTest)
			glDisable(GL_DEPTH_TEST);

		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);

		if (!depthTest)
			glEnable(GL_DEPTH_TEST);
	}

}