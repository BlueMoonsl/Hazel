#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Renderer/Renderer.h"

#include <string>
#include <glm/glm.hpp>

namespace Hazel
{
	// 表示单个着色器Uniform变量
	struct HAZEL_API ShaderUniform
	{
		
	};

	// 表示一组着色器Uniform变量
	struct HAZEL_API ShaderUniformCollection
	{

	};

	// Uniform变量类型枚举
	enum class HAZEL_API UniformType
	{
		None = 0,
		Float,      // 单精度浮点
		Float2,     // 2分量浮点向量
		Float3,     // 3分量浮点向量
		Float4,     // 4分量浮点向量
		Matrix3x3,  // 3x3矩阵
		Matrix4x4,  // 4x4矩阵
		Int32,      // 32位有符号整数
		Uint32      // 32位无符号整数
	};

	// 描述单个Uniform变量的声明
	struct HAZEL_API UniformDecl
	{
		UniformType Type;         // Uniform类型
		std::ptrdiff_t Offset;    // 在缓冲区中的偏移
		std::string Name;         // Uniform名称
	};

	// Uniform缓冲区的抽象，存储Uniform数据和声明
	struct HAZEL_API UniformBuffer
	{
		// TODO: 当前表示一个已打包的字节缓冲区，主要为OpenGL设计。
		// 需要为其他渲染API重新设计，并考虑内存对齐。
		// 注意：与OpenGL的Uniform Buffer无关，仅为CPU侧抽象。
		byte* Buffer;                        // 原始数据缓冲区
		std::vector<UniformDecl> Uniforms;   // Uniform声明列表
	};

	// Uniform缓冲区基类，定义访问接口
	struct UniformBufferBase
	{
		virtual const byte* GetBuffer() const = 0;
		virtual const UniformDecl* GetUniforms() const = 0;
		virtual unsigned int GetUniformCount() const = 0;
	};

	// 模板化的Uniform缓冲区声明，N为缓冲区大小，U为Uniform数量
	template<unsigned int N, unsigned int U>
	struct UniformBufferDeclaration : public UniformBufferBase
	{
		byte Buffer[N];              // 固定大小的缓冲区
		UniformDecl Uniforms[U];     // 固定数量的Uniform声明
		std::ptrdiff_t Cursor = 0;   // 当前写入位置

		virtual const byte* GetBuffer() const override { return Buffer; }
		virtual const UniformDecl* GetUniforms() const override { return Uniforms; }
		virtual unsigned int GetUniformCount() const { return U; }

		// 泛型Push方法，向缓冲区添加数据
		template<typename T>
		void Push(const std::string& name, const T& data) {}

		// float类型特化，添加float数据到缓冲区
		template<>
		void Push(const std::string& name, const float& data)
		{
			Uniforms[0] = { UniformType::Float, Cursor, name };
			memcpy(Buffer + Cursor, &data, sizeof(float));
			Cursor += sizeof(float);
		}

		// glm::vec4类型特化，添加vec4数据到缓冲区
		template<>
		void Push(const std::string& name, const glm::vec4& data)
		{
			Uniforms[0] = { UniformType::Float4, Cursor, name };
			memcpy(Buffer + Cursor, glm::value_ptr(data), sizeof(glm::vec4));
			Cursor += sizeof(glm::vec4);
		}

	};

	class HAZEL_API Shader
	{
	public:
		virtual void Bind() = 0; // 绑定着色器
		virtual void UploadUniformBuffer(const UniformBufferBase& uniformBuffer) = 0; // 上传Uniform缓冲区

		// 创建Shader实例，filepath为着色器文件路径
		// 目前仅支持单文件，未来可扩展为资源对象+元数据
		static Shader* Create(const std::string& filepath);
	};

}