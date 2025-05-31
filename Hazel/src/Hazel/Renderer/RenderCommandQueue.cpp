#include "hzpch.h"
#include "RenderCommandQueue.h"

#define HZ_RENDER_TRACE(...) HZ_CORE_TRACE(__VA_ARGS__)

namespace Hazel {

	// 构造函数，分配 10MB 的命令缓冲区并初始化
	RenderCommandQueue::RenderCommandQueue()
	{
		m_CommandBuffer = new unsigned char[10 * 1024 * 1024]; // 10MB 缓冲区
		m_CommandBufferPtr = m_CommandBuffer;
		memset(m_CommandBuffer, 0, 10 * 1024 * 1024);
	}

	// 析构函数，释放命令缓冲区
	RenderCommandQueue::~RenderCommandQueue()
	{
		delete[] m_CommandBuffer;
	}

	// 分配一条渲染命令到缓冲区、fn: 渲染命令函数指针、size: 命令数据大小、返回：可写入命令数据的内存指针
	void* RenderCommandQueue::Allocate(RenderCommandFn fn, unsigned int size)
	{
		// TODO: 需要处理内存对齐
		// 写入函数指针
		*(RenderCommandFn*)m_CommandBufferPtr = fn;
		m_CommandBufferPtr += sizeof(RenderCommandFn);

		// 写入命令数据大小
		*(int*)m_CommandBufferPtr = size;
		m_CommandBufferPtr += sizeof(unsigned int);

		// 分配命令数据内存
		void* memory = m_CommandBufferPtr;
		m_CommandBufferPtr += size;

		m_CommandCount++;
		return memory;
	}

	// 执行缓冲区中的所有渲染命令
	void RenderCommandQueue::Execute()
	{
		HZ_RENDER_TRACE("RenderCommandQueue::Execute -- {0} commands, {1} bytes", m_CommandCount, (m_CommandBufferPtr - m_CommandBuffer));

		byte* buffer = m_CommandBuffer;

		// 依次读取并执行每条命令
		for (unsigned int i = 0; i < m_CommandCount; i++)
		{
			// 读取函数指针
			RenderCommandFn function = *(RenderCommandFn*)buffer;
			buffer += sizeof(RenderCommandFn);

			// 读取命令数据大小
			unsigned int size = *(unsigned int*)buffer;
			buffer += sizeof(unsigned int);

			// 执行命令
			function(buffer);
			buffer += size;
		}

		// 重置缓冲区指针和命令计数
		m_CommandBufferPtr = m_CommandBuffer;
		m_CommandCount = 0;
	}

}