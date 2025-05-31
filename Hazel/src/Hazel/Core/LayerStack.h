#pragma once

#include "Hazel/Core/Base.h"
#include "Layer.h"

#include <vector>

namespace Hazel {

	class HAZEL_API LayerStack
	{
	public:
		LayerStack();
		~LayerStack();

		// 普通Layer用于游戏或应用的逻辑，OverLayer用于渲染在所有Layer之上的U内容，如UI、调试窗口等
		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* overlay);

		std::vector<Layer*>::iterator begin() { return m_Layers.begin(); }
		std::vector<Layer*>::iterator end() { return m_Layers.end(); }
	private:
		std::vector<Layer*> m_Layers;
		unsigned int m_LayerInsertIndex = 0;
	};

}