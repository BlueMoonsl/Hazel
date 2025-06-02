#pragma once

#include <glm/glm.hpp>

#include "Hazel/Renderer/RendererAPI.h"

namespace Hazel {

	// 帧缓冲格式
	enum class FramebufferFormat
	{
		None = 0,
		RGBA8 = 1,
		RGBA16F = 2
	};

	struct FramebufferSpecification
	{
		uint32_t Width = 1280;
		uint32_t Height = 720;
		glm::vec4 ClearColor;
		FramebufferFormat Format;

		// SwapChainTarget = screen buffer (i.e. no framebuffer)
		bool SwapChainTarget = false;
	};

	// 帧缓冲类
	class Framebuffer
	{
	public:
		virtual ~Framebuffer() {}
		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual void BindTexture(uint32_t slot = 0) const = 0;			// 绑定纹理，默认插槽0

		virtual RendererID GetRendererID() const = 0;					// 获取渲染器标识符
		virtual RendererID GetColorAttachmentRendererID() const = 0;	// 获取颜色附着标识符
		virtual RendererID GetDepthAttachmentRendererID() const = 0;	// 获取深度附着标识符
	
		virtual const FramebufferSpecification& GetSpecification() const = 0;

		static Ref<Framebuffer> Create(const FramebufferSpecification& spec);
	};

	// 帧缓冲池：管理帧缓冲区对象的对象的生命周期，通过复用帧缓冲对象，能够减少频繁创建和销毁帧缓冲带来的性能损耗。
	class FramebufferPool final
	{
	public:
		FramebufferPool(uint32_t maxFBs = 32);									// 默认最大帧缓冲数量为32
		~FramebufferPool();

		std::weak_ptr<Framebuffer> AllocateBuffer();							// 从对象池中分配一个可用的帧缓冲
		void Add(std::weak_ptr<Framebuffer> framebuffer);								// 添加帧缓冲到对象池

		const std::vector<std::weak_ptr<Framebuffer>>& GetAll() const { return m_Pool; }		// 获取对象池数组

		inline static FramebufferPool* GetGlobal() { return s_Instance; }		// 获取实例
	private:
		std::vector<std::weak_ptr<Framebuffer>> m_Pool;									// 对象池数组

		static FramebufferPool* s_Instance;										// 类的唯一实例，使用单例模式
	};

}