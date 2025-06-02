#pragma once

#include "hzpch.h"

namespace Hazel {

	// 事件类型枚举，定义了所有可能的事件类型
	enum class EventType
	{
		None = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		AppTick, AppUpdate, AppRender,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
	};

	// 事件分类枚举，用于对事件进行分组
	enum EventCategory
	{
		None = 0,
		EventCategoryApplication    = BIT(0),   // 应用相关事件
		EventCategoryInput          = BIT(1),   // 输入相关事件
		EventCategoryKeyboard       = BIT(2),   // 键盘相关事件
		EventCategoryMouse          = BIT(3),   // 鼠标相关事件
		EventCategoryMouseButton    = BIT(4)    // 鼠标按键相关事件
	};

	// 辅助宏：快速实现事件类型相关的虚函数
#define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::##type; }\
								virtual EventType GetEventType() const override { return GetStaticType(); }\
								virtual const char* GetName() const override { return #type; }

	// 辅助宏：快速实现事件分类相关的虚函数
#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }

	// 事件基类，所有事件需继承此类
	class Event
	{
	public:
		bool Handled = false; // 事件是否已被处理

		virtual EventType GetEventType() const = 0;      // 获取事件类型
		virtual const char* GetName() const = 0;         // 获取事件名称
		virtual int GetCategoryFlags() const = 0;        // 获取事件分类标志
		virtual std::string ToString() const { return GetName(); } // 事件转为字符串

		// 判断事件是否属于某一分类
		inline bool IsInCategory(EventCategory category)
		{
			return GetCategoryFlags() & category;
		}
	};

	// 事件分发器，用于分发和处理事件
	class EventDispatcher
	{
		// 事件处理函数类型定义
		template<typename T>
		using EventFn = std::function<bool(T&)>;
	public:
		// 构造函数，传入待分发的事件
		EventDispatcher(Event& event)
			: m_Event(event)
		{
		}

		// 分发事件，如果类型匹配则调用处理函数
		template<typename T>
		bool Dispatch(EventFn<T> func)
		{
			if (m_Event.GetEventType() == T::GetStaticType())
			{
				m_Event.Handled = func(*(T*)&m_Event);
				return true;
			}
			return false;
		}
	private:
		Event& m_Event; // 当前分发的事件
	};

	// 重载输出运算符，便于输出事件信息
	inline std::ostream& operator<<(std::ostream& os, const Event& e)
	{
		return os << e.ToString();
	}
}

