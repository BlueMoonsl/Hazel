#pragma once

#include "hzpch.h"

namespace Hazel {

	class HAZEL_API RenderCommandQueue
	{
	public:
		typedef void(*RenderCommandFn)(void*);

		RenderCommandQueue();
		~RenderCommandQueue();

		// 分配一块内存用于存储一个渲染命令
		void* Allocate(RenderCommandFn func, unsigned int size);
		// 执行队列中的所有渲染命令
		void Execute();
	private:
		unsigned char* m_CommandBuffer;		// 命令缓冲区起始指针
		unsigned char* m_CommandBufferPtr;	// 当前写入位置指针
		unsigned int m_CommandCount = 0;	// 当前命令数量
	};



}