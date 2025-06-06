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
		Float16 = 3
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

		virtual TextureFormat GetFormat() const = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetMipLevelCount() const = 0;

		virtual RendererID GetRendererID() const = 0;		// 获取底层渲染 API 的纹理 ID

		static uint32_t GetBPP(TextureFormat format);		// 根据格式获取每像素字节数
		static uint32_t CalculateMipMapCount(uint32_t width, uint32_t height);
	};

	// 2D 纹理类
	class Texture2D : public Texture
	{
	public:
		static Ref<Texture2D> Create(TextureFormat format, uint32_t width, uint32_t height, TextureWrap wrap = TextureWrap::Clamp);
		static Ref<Texture2D> Create(const std::string& path, bool srgb = false);

		// 锁定/解锁纹理数据（用于写入）
		virtual void Lock() = 0;
		virtual void Unlock() = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;	// 调整纹理大小
		virtual Buffer GetWriteableBuffer() = 0;					// 获取可写缓冲区

		virtual bool Loaded() const = 0;

		virtual const std::string& GetPath() const = 0;				// 获取纹理路径
	};

	// 立方体纹理类
	class TextureCube : public Texture
	{
	public:
		static Ref<TextureCube> Create(TextureFormat format, uint32_t width, uint32_t height);
		static Ref<TextureCube> Create(const std::string& path);
	
		virtual const std::string& GetPath() const = 0;			// 获取纹理路径
	};

}