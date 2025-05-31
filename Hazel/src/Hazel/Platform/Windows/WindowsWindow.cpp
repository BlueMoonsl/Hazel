#include "hzpch.h"
#include <glad/glad.h>
#include "WindowsWindow.h"

#include "Hazel/Core/Events/ApplicationEvent.h"
#include "Hazel/Core/Events/KeyEvent.h"
#include "Hazel/Core/Events/MouseEvent.h"

#include <imgui.h>

namespace Hazel {

	// GLFW 错误回调函数
	static void GLFWErrorCallback(int error, const char* description)
	{
		HZ_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	}
	
	// GLFW 初始化标志，确保只初始化一次
	static bool s_GLFWInitialized = false;

	// 创建窗口的工厂方法
	Window* Window::Create(const WindowProps& props)
	{
		return new WindowsWindow(props);
	}

	// Windows 平台窗口构造函数
	WindowsWindow::WindowsWindow(const WindowProps& props)
	{
		Init(props);
	}

	// 析构函数
	WindowsWindow::~WindowsWindow()
	{
	}

	// 初始化窗口
	void WindowsWindow::Init(const WindowProps& props)
	{
		m_Data.Title = props.Title;
		m_Data.Width = props.Width;
		m_Data.Height = props.Height;

		HZ_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

		// 初始化 GLFW（只执行一次）
		if (!s_GLFWInitialized)
		{
			// TODO: 程序关闭时调用 glfwTerminate
			int success = glfwInit();
			HZ_CORE_ASSERT(success, "Could not intialize GLFW!");
			glfwSetErrorCallback(GLFWErrorCallback);

			s_GLFWInitialized = true;
		}

		// 创建 GLFW 窗口
		m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, m_Data.Title.c_str(), nullptr, nullptr);
		glfwMakeContextCurrent(m_Window);
		// 初始化 OpenGL 函数指针
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		HZ_CORE_ASSERT(status, "Failed to initialize Glad!");
		// 设置用户指针，便于回调中访问窗口数据
		glfwSetWindowUserPointer(m_Window, &m_Data);
		SetVSync(true);

		// 设置各种 GLFW 回调
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
		{
			auto& data = *((WindowData*)glfwGetWindowUserPointer(window));
			WindowResizeEvent event((unsigned int)width, (unsigned int)height);
			data.EventCallback(event);
			data.Width = width;
			data.Height = height;
		});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
		{
			auto& data = *((WindowData*)glfwGetWindowUserPointer(window));
			WindowCloseEvent event;
			data.EventCallback(event);
		});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			auto& data = *((WindowData*)glfwGetWindowUserPointer(window));
			switch (action)
			{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(key, 0);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(key);
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(key, 1);
					data.EventCallback(event);
					break;
				}
			}
		});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int codepoint)
		{
			auto& data = *((WindowData*)glfwGetWindowUserPointer(window));
			KeyTypedEvent event((int)codepoint);
			data.EventCallback(event);
		});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
		{
			auto& data = *((WindowData*)glfwGetWindowUserPointer(window));
			switch (action)
			{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(button);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(button);
					data.EventCallback(event);
					break;
				}
			}
		});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			auto& data = *((WindowData*)glfwGetWindowUserPointer(window));
			MouseScrolledEvent event((float)xOffset, (float)yOffset);
			data.EventCallback(event);
		});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double x, double y)
		{
			auto& data = *((WindowData*)glfwGetWindowUserPointer(window));
			MouseMovedEvent event((float)x, (float)y);
			data.EventCallback(event);
		});

		// 初始化 ImGui 鼠标光标
		m_ImGuiMouseCursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
		m_ImGuiMouseCursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
		m_ImGuiMouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);   // FIXME: GLFW 没有此类型
		m_ImGuiMouseCursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
		m_ImGuiMouseCursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
		m_ImGuiMouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);  // FIXME: GLFW 没有此类型
		m_ImGuiMouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);  // FIXME: GLFW 没有此类型
		m_ImGuiMouseCursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
	}

	// 关闭窗口，资源释放（未实现）
	void WindowsWindow::Shutdown()
	{
		
	}

	// 每帧更新窗口（处理事件和交换缓冲）
	void WindowsWindow::OnUpdate()
	{
		glfwPollEvents();
		glfwSwapBuffers(m_Window);

		// 设置 ImGui 鼠标光标
		ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
		glfwSetCursor(m_Window, m_ImGuiMouseCursors[imgui_cursor] ? m_ImGuiMouseCursors[imgui_cursor] : m_ImGuiMouseCursors[ImGuiMouseCursor_Arrow]);
		glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	// 设置垂直同步
	void WindowsWindow::SetVSync(bool enabled)
	{
		if (enabled)
			glfwSwapInterval(1);
		else
			glfwSwapInterval(0);

		m_Data.VSync = enabled;
	}

	// 查询是否启用垂直同步
	bool WindowsWindow::IsVSync() const
	{
		return m_Data.VSync;
	}

}
