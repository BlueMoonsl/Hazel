#pragma once

#include "RenderCommandQueue.h"
#include "RendererAPI.h"

namespace Hazel {

	class HAZEL_API Renderer
	{
	public:
		// 渲染命令函数指针类型
		typedef void(*RenderCommandFn)(void*);							
		
		static void Clear();											// 渲染命令：清空帧缓冲
		static void Clear(float r, float g, float b, float a = 1.0f);  	// 渲染命令：以指定颜色清空帧缓冲
		static void SetClearColor(float r, float g, float b, float a);	// 设置清空颜色

		static void DrawIndexed(unsigned int count);					// 绘制索引图元

		static void ClearMagenta();										// 清空为洋红色（调试用）

		static void Init();

		// 提交渲染命令到命令队列
		static void* Submit(RenderCommandFn fn, unsigned int size)
		{
			return s_Instance->m_CommandQueue.Allocate(fn, size);
		}

		// 等待并执行渲染命令队列
		void WaitAndRender();

		// 获取渲染器单例实例
		inline static Renderer& Get() { return *s_Instance; }
	private:
		static Renderer* s_Instance;           // 渲染器单例指针

		RenderCommandQueue m_CommandQueue;     // 渲染命令队列
	};

}

// 宏工具：拼接唯一标识符
#define HZ_RENDER_PASTE2(a, b) a ## b
#define HZ_RENDER_PASTE(a, b) HZ_RENDER_PASTE2(a, b)
#define HZ_RENDER_UNIQUE(x) HZ_RENDER_PASTE(x, __LINE__)

// HZ_RENDER 宏：无参数渲染命令，自动生成命令结构体并提交到渲染队列
#define HZ_RENDER(code) \
    struct HZ_RENDER_UNIQUE(HZRenderCommand) \
    {\
        static void Execute(void*)\
        {\
            code\
        }\
    };\
	{\
		auto mem = ::Hazel::Renderer::Submit(HZ_RENDER_UNIQUE(HZRenderCommand)::Execute, sizeof(HZ_RENDER_UNIQUE(HZRenderCommand)));\
		new (mem) HZ_RENDER_UNIQUE(HZRenderCommand)();\
	}\

// HZ_RENDER_1 宏：带一个参数的渲染命令
#define HZ_RENDER_1(arg0, code) \
	do {\
    struct HZ_RENDER_UNIQUE(HZRenderCommand) \
    {\
		HZ_RENDER_UNIQUE(HZRenderCommand)(typename ::std::remove_const<typename ::std::remove_reference<decltype(arg0)>::type>::type arg0) \
		: arg0(arg0) {}\
		\
        static void Execute(void* argBuffer)\
        {\
			auto& arg0 = ((HZ_RENDER_UNIQUE(HZRenderCommand)*)argBuffer)->arg0;\
            code\
        }\
		\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg0)>::type>::type arg0;\
    };\
	{\
		auto mem = ::Hazel::Renderer::Submit(HZ_RENDER_UNIQUE(HZRenderCommand)::Execute, sizeof(HZ_RENDER_UNIQUE(HZRenderCommand)));\
		new (mem) HZ_RENDER_UNIQUE(HZRenderCommand)(arg0);\
	} } while(0)

// HZ_RENDER_2 宏：带两个参数的渲染命令
#define HZ_RENDER_2(arg0, arg1, code) \
    struct HZ_RENDER_UNIQUE(HZRenderCommand) \
    {\
		HZ_RENDER_UNIQUE(HZRenderCommand)(typename ::std::remove_const<typename ::std::remove_reference<decltype(arg0)>::type>::type arg0,\
											typename ::std::remove_const<typename ::std::remove_reference<decltype(arg1)>::type>::type arg1) \
		: arg0(arg0), arg1(arg1) {}\
		\
        static void Execute(void* argBuffer)\
        {\
			auto& arg0 = ((HZ_RENDER_UNIQUE(HZRenderCommand)*)argBuffer)->arg0;\
			auto& arg1 = ((HZ_RENDER_UNIQUE(HZRenderCommand)*)argBuffer)->arg1;\
            code\
        }\
		\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg0)>::type>::type arg0;\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg1)>::type>::type arg1;\
    };\
	{\
		auto mem = ::Hazel::Renderer::Submit(HZ_RENDER_UNIQUE(HZRenderCommand)::Execute, sizeof(HZ_RENDER_UNIQUE(HZRenderCommand)));\
		new (mem) HZ_RENDER_UNIQUE(HZRenderCommand)(arg0, arg1);\
	}\

// HZ_RENDER_3 宏：带三个参数的渲染命令
#define HZ_RENDER_3(arg0, arg1, arg2, code) \
    struct HZ_RENDER_UNIQUE(HZRenderCommand) \
    {\
		HZ_RENDER_UNIQUE(HZRenderCommand)(typename ::std::remove_const<typename ::std::remove_reference<decltype(arg0)>::type>::type arg0,\
											typename ::std::remove_const<typename ::std::remove_reference<decltype(arg1)>::type>::type arg1,\
											typename ::std::remove_const<typename ::std::remove_reference<decltype(arg2)>::type>::type arg2) \
		: arg0(arg0), arg1(arg1), arg2(arg2) {}\
		\
        static void Execute(void* argBuffer)\
        {\
			auto& arg0 = ((HZ_RENDER_UNIQUE(HZRenderCommand)*)argBuffer)->arg0;\
			auto& arg1 = ((HZ_RENDER_UNIQUE(HZRenderCommand)*)argBuffer)->arg1;\
			auto& arg2 = ((HZ_RENDER_UNIQUE(HZRenderCommand)*)argBuffer)->arg2;\
            code\
        }\
		\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg0)>::type>::type arg0;\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg1)>::type>::type arg1;\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg2)>::type>::type arg2;\
    };\
	{\
		auto mem = ::Hazel::Renderer::Submit(HZ_RENDER_UNIQUE(HZRenderCommand)::Execute, sizeof(HZ_RENDER_UNIQUE(HZRenderCommand)));\
		new (mem) HZ_RENDER_UNIQUE(HZRenderCommand)(arg0, arg1, arg2);\
	}\

// HZ_RENDER_4 宏：带四个参数的渲染命令
#define HZ_RENDER_4(arg0, arg1, arg2, arg3, code) \
    struct HZ_RENDER_UNIQUE(HZRenderCommand) \
    {\
		HZ_RENDER_UNIQUE(HZRenderCommand)(typename ::std::remove_const<typename ::std::remove_reference<decltype(arg0)>::type>::type arg0,\
											typename ::std::remove_const<typename ::std::remove_reference<decltype(arg1)>::type>::type arg1,\
											typename ::std::remove_const<typename ::std::remove_reference<decltype(arg2)>::type>::type arg2,\
											typename ::std::remove_const<typename ::std::remove_reference<decltype(arg3)>::type>::type arg3)\
		: arg0(arg0), arg1(arg1), arg2(arg2), arg3(arg3) {}\
		\
        static void Execute(void* argBuffer)\
        {\
			auto& arg0 = ((HZ_RENDER_UNIQUE(HZRenderCommand)*)argBuffer)->arg0;\
			auto& arg1 = ((HZ_RENDER_UNIQUE(HZRenderCommand)*)argBuffer)->arg1;\
			auto& arg2 = ((HZ_RENDER_UNIQUE(HZRenderCommand)*)argBuffer)->arg2;\
			auto& arg3 = ((HZ_RENDER_UNIQUE(HZRenderCommand)*)argBuffer)->arg3;\
            code\
        }\
		\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg0)>::type>::type arg0;\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg1)>::type>::type arg1;\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg2)>::type>::type arg2;\
		typename ::std::remove_const<typename ::std::remove_reference<decltype(arg3)>::type>::type arg3;\
    };\
	{\
		auto mem = Renderer::Submit(HZ_RENDER_UNIQUE(HZRenderCommand)::Execute, sizeof(HZ_RENDER_UNIQUE(HZRenderCommand)));\
		new (mem) HZ_RENDER_UNIQUE(HZRenderCommand)(arg0, arg1, arg2, arg3);\
	}

// 以下宏用于在成员函数中便捷提交渲染命令（自动传递 this 指针）
#define HZ_RENDER_S(code) auto self = this;\
	HZ_RENDER_1(self, code)

#define HZ_RENDER_S1(arg0, code) auto self = this;\
	HZ_RENDER_2(self, arg0, code)

#define HZ_RENDER_S2(arg0, arg1, code) auto self = this;\
	HZ_RENDER_3(self, arg0, arg1, code)

#define HZ_RENDER_S3(arg0, arg1, arg2, code) auto self = this;\
	HZ_RENDER_4(self, arg0, arg1, arg2, code)