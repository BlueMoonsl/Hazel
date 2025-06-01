#include "hzpch.h"
#include "WindowsInput.h"
#include "WindowsWindow.h"

#include "Hazel/Core/Application.h"

#include <GLFW/glfw3.h>

namespace Hazel {

	Input* Input::s_Instance = new WindowsInput;

	// 检查指定键是否被按下（包括长按重复）
	bool WindowsInput::IsKeyPressedImpl(int keycode)
	{
		auto& window = static_cast<WindowsWindow&>(Application::Get().GetWindow());
		// 获取键盘按键状态
		auto state = glfwGetKey(static_cast<GLFWwindow*>(window.GetNativeWindow()), keycode);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	// 检查指定鼠标按钮是否被按下
	bool WindowsInput::IsMouseButtonPressedImpl(int button)
	{
		auto& window = static_cast<WindowsWindow&>(Application::Get().GetWindow());
		// 获取鼠标按钮状态
		auto state = glfwGetMouseButton(static_cast<GLFWwindow*>(window.GetNativeWindow()), button);
		return state == GLFW_PRESS;
	}

	// 获取鼠标当前的 X 坐标
	float WindowsInput::GetMouseXImpl()
	{
		auto& window = static_cast<WindowsWindow&>(Application::Get().GetWindow());
		double xpos, ypos;
		// 获取鼠标当前位置
		glfwGetCursorPos(static_cast<GLFWwindow*>(window.GetNativeWindow()), &xpos, &ypos);
		return (float)xpos;
	}

	// 获取鼠标当前的 Y 坐标
	float WindowsInput::GetMouseYImpl()
	{
		auto& window = static_cast<WindowsWindow&>(Application::Get().GetWindow());
		double xpos, ypos;
		// 获取鼠标当前位置
		glfwGetCursorPos(static_cast<GLFWwindow*>(window.GetNativeWindow()), &xpos, &ypos);
		return (float)ypos;
	}

}