#pragma once

#include <functional>

#include "Hazel/Core/Base.h"
#include "Hazel/Core/Events/Event.h"

namespace Hazel {

	// 窗口信息结构体
	struct WindowProps
	{
		std::string Title;
		unsigned int Width;
		unsigned int Height;

		WindowProps(const std::string& title = "Hazel Engine",
			        unsigned int width = 1280,
			        unsigned int height = 720)
			: Title(title), Width(width), Height(height)
		{
		}
	};

	// 桌面系统窗口接口，所有平台窗口需实现此接口
	class Window : public RefCounted
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		virtual ~Window() {}

		virtual void OnUpdate() = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual std::pair<uint32_t, uint32_t> GetSize() const = 0;
		virtual std::pair<float, float> GetWindowPos() const = 0;

		// 窗口属性
		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;		// 设置事件回调函数
		virtual void SetVSync(bool enabled) = 0;								// 设置垂直同步
		virtual bool IsVSync() const = 0;										// 是否启用垂直同步

		virtual void* GetNativeWindow() const = 0;								// 获取原生窗口指针（如 GLFWwindow*）

		static Window* Create(const WindowProps& props = WindowProps());
	};

}