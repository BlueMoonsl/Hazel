#pragma once

#include "Hazel/Renderer/Renderer.h"

namespace Hazel {

	// 顶点缓冲区
	class HAZEL_API VertexBuffer
	{
	public:
		virtual ~VertexBuffer() {}
																										
		virtual void SetData(void* buffer, unsigned int size, unsigned int offset = 0) = 0;	 // 设置缓冲区数据
		virtual void Bind() const = 0; 														 // 绑定缓冲区到渲染管线

		virtual unsigned int GetSize() const = 0;											 // 获取缓冲区大小
		virtual RendererID GetRendererID() const = 0;										 // 获取底层API的缓冲区ID

		static VertexBuffer* Create(unsigned int size = 0);
	};

	// 索引缓冲区
	class HAZEL_API IndexBuffer
	{
	public:
		virtual ~IndexBuffer() {}

		virtual void SetData(void* buffer, unsigned int size, unsigned int offset = 0) = 0;
		virtual void Bind() const = 0;

		virtual uint32_t GetCount() const = 0;												 // 获取索引数量

		virtual unsigned int GetSize() const = 0;
		virtual RendererID GetRendererID() const = 0;

		static IndexBuffer* Create(unsigned int size = 0);
	};

}