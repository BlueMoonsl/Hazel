#pragma once

#include "Hazel/Core/Base.h"

namespace Hazel {

	// 通用缓冲区结构体，用于存储任意二进制数据
	struct Buffer
	{
		byte* Data;        // 数据指针
		uint32_t Size;     // 数据大小（字节）

		// 默认构造函数，初始化为空
		Buffer()
			: Data(nullptr), Size(0)
		{
		}

		// 构造函数，使用外部数据和大小初始化
		Buffer(byte* data, uint32_t size)
			: Data(data), Size(size)
		{
		}

		static Buffer Copy(void* data, uint32_t size)
		{
			Buffer buffer;
			buffer.Allocate(size);
			memcpy(buffer.Data, data, size);
			return buffer;
		}

		// 分配指定大小的缓冲区，自动释放旧数据
		void Allocate(uint32_t size)
		{
			delete[] Data;
			Data = nullptr;

			if (size == 0)
				return;

			Data = new byte[size];
			Size = size;
		}

		// 将缓冲区内容全部置零
		void ZeroInitialize()
		{
			if (Data)
				memset(Data, 0, Size);
		}

		// 向缓冲区写入数据，支持偏移
		void Write(void* data, uint32_t size, uint32_t offset = 0)
		{
			HZ_CORE_ASSERT(offset + size <= Size, "Buffer overflow!");
			memcpy(Data + offset, data, size);
		}

		// 判断缓冲区是否有效（Data 非空）
		operator bool() const
		{
			return Data;
		}

		// 下标访问运算符（可写）
		byte& operator[](int index)
		{
			return Data[index];
		}

		// 下标访问运算符（只读）
		byte operator[](int index) const
		{
			return Data[index];
		}

		// 将缓冲区强制转换为指定类型指针
		template<typename T>
		T* As()
		{
			return (T*)Data;
		}

		// 获取缓冲区大小
		inline uint32_t GetSize() const { return Size; }
	};

}
