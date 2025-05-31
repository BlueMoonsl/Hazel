#include "hzpch.h"
#include "ImGuiLayer.h"

#include "imgui.h"

// ImGui GLFW 和 OpenGL3 后端实现
#define IMGUI_IMPL_API
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "Hazel/Core/Application.h"
#include <GLFW/glfw3.h>

#include "Hazel/Renderer/Renderer.h"

namespace Hazel {

	ImGuiLayer::ImGuiLayer()
	{

	}

	ImGuiLayer::ImGuiLayer(const std::string& name)
	{

	}

	ImGuiLayer::~ImGuiLayer()
	{

	}

	// ImGuiLayer 附加到应用时调用，初始化ImGui
	void ImGuiLayer::OnAttach()
	{
		// 设置 Dear ImGui 上下文
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // 启用键盘控制
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // 启用手柄控制
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // 启用窗口停靠
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // 启用多视口/平台窗口
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

		// 设置 ImGui 样式
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();

		// 如果启用多视口，调整窗口圆角和背景透明度
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		// 获取应用窗口的原生句柄
		Application& app = Application::Get();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());

		// 初始化 ImGui 平台/渲染器绑定
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 410");
	}

	// ImGuiLayer 从应用分离时调用，清理ImGui
	void ImGuiLayer::OnDetach()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	// 每帧开始时调用，准备ImGui新帧
	void ImGuiLayer::Begin()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	// 每帧结束时调用，渲染ImGui内容
	void ImGuiLayer::End()
	{
		ImGuiIO& io = ImGui::GetIO();
		Application& app = Application::Get();
		// 设置ImGui显示区域为应用窗口大小
		io.DisplaySize = ImVec2(app.GetWindow().GetWidth(), app.GetWindow().GetHeight());

		// 渲染ImGui
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// 如果启用多视口，更新和渲染平台窗口
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}

	// ImGui 渲染回调（可自定义ImGui内容）
	void ImGuiLayer::OnImGuiRender()
	{
	}

}