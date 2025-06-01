#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Core/Buffer.h"
#include "RendererAPI.h"

namespace Hazel {

	// 纹理格式枚举
	enum class TextureFormat
	{
		None = 0,
		RGB = 1,   // 3 通道
		RGBA = 2,  // 4 通道
	};

	// 纹理环绕方式枚举
	enum class TextureWrap
	{
		None = 0,
		Clamp = 1,   // 边缘拉伸
		Repeat = 2   // 重复平铺
	};

	// 纹理基类，所有纹理类型的抽象接口
	class Texture
	{
	public:
		virtual ~Texture() {}

		virtual void Bind(uint32_t slot = 0) const = 0;		// 绑定纹理到指定插槽

		virtual RendererID GetRendererID() const = 0;		// 获取底层渲染 API 的纹理 ID

		static uint32_t GetBPP(TextureFormat format);		// 根据格式获取每像素字节数
	};

	// 2D 纹理类
	class Texture2D : public Texture
	{
	public:
		// 创建纹理
		static Texture2D* Create(TextureFormat format, unsigned int width, unsigned int height, TextureWrap wrap = TextureWrap::Clamp);
		static Texture2D* Create(const std::string& path, bool srgb = false);

		virtual TextureFormat GetFormat() const = 0;		// 获取纹理格式
		virtual uint32_t GetWidth() const = 0;				// 获取宽度
		virtual uint32_t GetHeight() const = 0;				// 获取高度

		// 锁定/解锁纹理数据（用于写入）
		virtual void Lock() = 0;
		virtual void Unlock() = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;	// 调整纹理大小
		virtual Buffer GetWriteableBuffer() = 0;					// 获取可写缓冲区

		virtual const std::string& GetPath() const = 0;				// 获取纹理路径
	};

	// 立方体纹理类
	class TextureCube : public Texture
	{
	public:

		static TextureCube* Create(const std::string& path);	// 从文件创建立方体纹理

		virtual TextureFormat GetFormat() const = 0;			// 获取纹理格式
		virtual unsigned int GetWidth() const = 0;				// 获取宽度
		virtual unsigned int GetHeight() const = 0;				// 获取高度
	
		virtual const std::string& GetPath() const = 0;			// 获取纹理路径
	};

}