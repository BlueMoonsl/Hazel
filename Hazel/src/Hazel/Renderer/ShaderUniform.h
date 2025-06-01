#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Core/Log.h"

#include <string>
#include <vector>

namespace Hazel {

	// 着色器变量所属域（顶点/像素着色器）
	enum class ShaderDomain
	{
		None = 0, Vertex = 0, Pixel = 1
	};

	// 着色器 Uniform 变量声明的抽象基类
	class ShaderUniformDeclaration
	{
	private:
		friend class Shader;
		friend class OpenGLShader;
		friend class ShaderStruct;
	public:
		virtual const std::string& GetName() const = 0;      // 获取变量名
		virtual uint32_t GetSize() const = 0;                // 获取变量大小（字节）
		virtual uint32_t GetCount() const = 0;               // 获取变量数量（数组支持）
		virtual uint32_t GetOffset() const = 0;              // 获取变量在缓冲区中的偏移
		virtual ShaderDomain GetDomain() const = 0;          // 获取变量所属着色器域
	protected:
		virtual void SetOffset(uint32_t offset) = 0;         // 设置变量偏移（仅限内部使用）
	};

	typedef std::vector<ShaderUniformDeclaration*> ShaderUniformList;

	// Uniform 缓冲区声明的抽象基类
	class ShaderUniformBufferDeclaration
	{
	public:
		virtual const std::string& GetName() const = 0;                        // 获取缓冲区名
		virtual uint32_t GetRegister() const = 0;                              // 获取缓冲区寄存器槽
		virtual uint32_t GetSize() const = 0;                                  // 获取缓冲区总大小
		virtual const ShaderUniformList& GetUniformDeclarations() const = 0;   // 获取所有 Uniform 声明

		virtual ShaderUniformDeclaration* FindUniform(const std::string& name) = 0; // 查找指定名称的 Uniform
	};

	typedef std::vector<ShaderUniformBufferDeclaration*> ShaderUniformBufferList;

	// 着色器结构体声明（用于复合类型 Uniform）
	class ShaderStruct
	{
	private:
		friend class Shader;
	private:
		std::string m_Name;                                 // 结构体名称
		std::vector<ShaderUniformDeclaration*> m_Fields;    // 字段列表
		uint32_t m_Size;                                    // 结构体总大小
		uint32_t m_Offset;                                  // 结构体在缓冲区中的偏移
	public:
		ShaderStruct(const std::string& name)
			: m_Name(name), m_Size(0), m_Offset(0)
		{
		}

		// 添加字段，并自动计算偏移和结构体大小
		void AddField(ShaderUniformDeclaration* field)
		{
			m_Size += field->GetSize();
			uint32_t offset = 0;
			if (m_Fields.size())
			{
				ShaderUniformDeclaration* previous = m_Fields.back();
				offset = previous->GetOffset() + previous->GetSize();
			}
			field->SetOffset(offset);
			m_Fields.push_back(field);
		}

		inline void SetOffset(uint32_t offset) { m_Offset = offset; } // 设置结构体偏移

		inline const std::string& GetName() const { return m_Name; }
		inline uint32_t GetSize() const { return m_Size; }
		inline uint32_t GetOffset() const { return m_Offset; }
		inline const std::vector<ShaderUniformDeclaration*>& GetFields() const { return m_Fields; }
	};

	typedef std::vector<ShaderStruct*> ShaderStructList;

	// 着色器资源声明（如纹理、采样器等）的抽象基类
	class ShaderResourceDeclaration
	{
	public:
		virtual const std::string& GetName() const = 0;     // 获取资源名
		virtual uint32_t GetRegister() const = 0;           // 获取资源绑定槽
		virtual uint32_t GetCount() const = 0;              // 获取资源数量
	};

	typedef std::vector<ShaderResourceDeclaration*> ShaderResourceList;

}
