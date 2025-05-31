#pragma once

#include <memory>

namespace Hazel {

	void InitializeCore();
	void ShutdownCore();

}

// 调试模式启用断言
#ifdef HZ_DEBUG
	#define HZ_ENABLE_ASSERTS
#endif

// Windows 平台下的 DLL 导出/导入设置
#ifdef HZ_PLATFORM_WINDOWS
#if HZ_DYNAMIC_LINK
	#ifdef HZ_BUILD_DLL
		#define HAZEL_API __declspec(dllexport)
	#else
		#define HAZEL_API __declspec(dllimport)
	#endif
#else
	#define HAZEL_API
#endif
#else
	#error Hazel only supports Windows!
#endif

// 断言宏定义
#ifdef HZ_ENABLE_ASSERTS
	#define HZ_ASSERT(x, ...) { if(!(x)) { HZ_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
	#define HZ_CORE_ASSERT(x, ...) { if(!(x)) { HZ_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define HZ_ASSERT(x, ...)
	#define HZ_CORE_ASSERT(x, ...)
#endif

// 位运算辅助宏
#define BIT(x) (1 << x)

// 事件绑定宏
#define HZ_BIND_EVENT_FN(fn) std::bind(&##fn, this, std::placeholders::_1)