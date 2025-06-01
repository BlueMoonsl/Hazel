#pragma once

#include <memory>

namespace Hazel {

	// 框架初始化与关闭函数声明
	void InitializeCore();
	void ShutdownCore();

}

// 调试模式下启用断言
#ifdef HZ_DEBUG
	#define HZ_ENABLE_ASSERTS
#endif

// Windows 平台下的 DLL 导出/导入设置
#ifdef HZ_PLATFORM_WINDOWS
#if HZ_DYNAMIC_LINK
	#ifdef HZ_BUILD_DLL
		#define HAZEL_API __declspec(dllexport)   // 构建 DLL 时导出符号
	#else
		#define HAZEL_API __declspec(dllimport)   // 使用 DLL 时导入符号
	#endif
#else
	#define HAZEL_API
#endif
#else
	#error Hazel only supports Windows! // 只支持 Windows 平台
#endif

// 断言宏定义（调试模式下有效）
#ifdef HZ_ENABLE_ASSERTS
	#define HZ_ASSERT_NO_MESSAGE(condition) { if(!(condition)) { HZ_ERROR("Assertion Failed!"); __debugbreak(); } }
	#define HZ_ASSERT_MESSAGE(condition, ...) { if(!(condition)) { HZ_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }

	#define HZ_ASSERT_RESOLVE(arg1, arg2, macro, ...) macro

	// 支持带消息和不带消息的断言
	#define HZ_ASSERT(...) HZ_ASSERT_RESOLVE(__VA_ARGS__, HZ_ASSERT_MESSAGE, HZ_ASSERT_NO_MESSAGE)(__VA_ARGS__)
	#define HZ_CORE_ASSERT(...) HZ_ASSERT_RESOLVE(__VA_ARGS__, HZ_ASSERT_MESSAGE, HZ_ASSERT_NO_MESSAGE)(__VA_ARGS__)
#else
	#define HZ_ASSERT(...)
	#define HZ_CORE_ASSERT(...)
#endif

// 位运算辅助宏，将 x 左移 1 位
#define BIT(x) (1 << x)

// 事件绑定宏，简化 std::bind 用法
#define HZ_BIND_EVENT_FN(fn) std::bind(&##fn, this, std::placeholders::_1)

// 智能指针类型别名
namespace Hazel {

	template<typename T>
	using Scope = std::unique_ptr<T>;      // 独占指针

	template<typename T>
	using Ref = std::shared_ptr<T>;        // 共享指针

	using byte = unsigned char;            // 字节类型别名

}