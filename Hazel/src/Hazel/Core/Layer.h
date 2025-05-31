#pragma once

#include "Hazel/Core/Base.h"

namespace Hazel {

	// Layer基类
	class HAZEL_API Layer
	{
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer();

		virtual void OnAttach() {}					// 附加到LayerStack时调用
		virtual void OnDetach() {}					// 从LayerStack分离时调用
		virtual void OnUpdate() {}					// 每帧更新时调用
		virtual void OnImGuiRender() {}				// ImGui渲染时回调
		virtual void OnEvent(Event& event) {}		// 事件处理时回调

		inline const std::string& GetName() const { return m_DebugName; }
	protected:
		std::string m_DebugName;
	};

}