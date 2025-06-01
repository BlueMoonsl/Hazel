#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Renderer/Renderer.h"

#include <string>
#include <glm/glm.hpp>

namespace Hazel
{
	struct HAZEL_API ShaderUniform
	{
		
	};

	struct HAZEL_API ShaderUniformCollection
	{

	};

	// Uniform变量类型枚举，描述支持的Uniform数据类型
	enum class HAZEL_API UniformType
	{
		None = 0,     // 无类型
		Float,        // 单精度浮点
		Float2,       // 2分量浮点向量
		Float3,       // 3分量浮点向量
		Float4,       // 4分量浮点向量
		Matrix3x3,    // 3x3矩阵
		Matrix4x4,    // 4x4矩阵
		Int32,        // 32位有符号整数
		Uint32        // 32位无符号整数
	};

	// 描述单个Uniform变量的声明信息
	struct HAZEL_API UniformDecl
	{
		UniformType Type;         // Uniform类型
		std::ptrdiff_t Offset;    // 在缓冲区中的字节偏移
		std::string Name;         // Uniform名称
	};

	// Uniform缓冲区的抽象，存储Uniform数据和声明（仅CPU侧，与OpenGL UBO无关）
	struct HAZEL_API UniformBuffer
	{
		byte* Buffer;                        // 原始数据缓冲区
		std::vector<UniformDecl> Uniforms;   // Uniform声明列表
	};

	// Uniform缓冲区基类，定义Uniform缓冲区的访问接口
	struct UniformBufferBase
	{
		virtual const byte* GetBuffer() const = 0;				// 获取原始数据缓冲区指针
		virtual const UniformDecl* GetUniforms() const = 0;		// 获取Uniform声明数组指针
		virtual unsigned int GetUniformCount() const = 0;		// 获取Uniform数量
	};

	// 模板化的Uniform缓冲区声明，N为缓冲区字节数，U为Uniform数量上限
	template<unsigned int N, unsigned int U>
	struct UniformBufferDeclaration : public UniformBufferBase
	{
		byte Buffer[N];              // 固定大小的字节缓冲区
		UniformDecl Uniforms[U];     // 固定数量的Uniform声明
		std::ptrdiff_t Cursor = 0;   // 当前写入偏移
		int Index = 0;               // 当前Uniform索引


		virtual const byte* GetBuffer() const override { return Buffer; }			
		virtual const UniformDecl* GetUniforms() const override { return Uniforms; }	
		virtual unsigned int GetUniformCount() const { return U; }						

		// 泛型Push方法，向缓冲区添加数据（需特化实现）
		template<typename T>
		void Push(const std::string& name, const T& data) {}

		// float类型特化，添加float数据到缓冲区
		template<>
		void Push(const std::string& name, const float& data)
		{
			Uniforms[Index++] = { UniformType::Float, Cursor, name };
			memcpy(Buffer + Cursor, &data, sizeof(float));
			Cursor += sizeof(float);
		}

		// glm::vec3类型特化，添加vec3数据到缓冲区
		template<>
		void Push(const std::string& name, const glm::vec3& data)
		{
			Uniforms[Index++] = { UniformType::Float3, Cursor, name };
			memcpy(Buffer + Cursor, glm::value_ptr(data), sizeof(glm::vec3));
			Cursor += sizeof(glm::vec3);
		}

		// glm::vec4类型特化，添加vec4数据到缓冲区
		template<>
		void Push(const std::string& name, const glm::vec4& data)
		{
			Uniforms[Index++] = { UniformType::Float4, Cursor, name };
			memcpy(Buffer + Cursor, glm::value_ptr(data), sizeof(glm::vec4));
			Cursor += sizeof(glm::vec4);
		}

		// glm::mat4类型特化，添加mat4数据到缓冲区
		template<>
		void Push(const std::string& name, const glm::mat4& data)
		{
			Uniforms[Index++] = { UniformType::Matrix4x4, Cursor, name };
			memcpy(Buffer + Cursor, glm::value_ptr(data), sizeof(glm::mat4));
			Cursor += sizeof(glm::mat4);
		}

	};

	// Shader类：抽象着色器对象，定义着色器的基本操作接口
	class HAZEL_API Shader
	{
	public:

		virtual void Reload() = 0;		// 重新加载着色
		virtual void Bind() = 0;		// 绑定着色器到渲染管线

		virtual void UploadUniformBuffer(const UniformBufferBase& uniformBuffer) = 0;	// 上传Uniform缓冲区到GPU

		virtual void SetFloat(const std::string& name, float value) = 0;				// 临时接口：设置float类型Uniform
		virtual void SetMat4(const std::string& name, const glm::mat4& value) = 0;		// 临时接口：设置mat4类型Uniform

		virtual const std::string& GetName() const = 0;			// 获取着色器名称

		static Shader* Create(const std::string& filepath);		// 创建Shader实例，filepath为着色器文件路径
		static std::vector<Shader*> s_AllShaders;				// 临时全局着色器列表（在资源管理器完善前使用）
	};

}