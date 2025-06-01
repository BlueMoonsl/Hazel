#pragma once

#include "Hazel/Renderer/ShaderUniform.h"

namespace Hazel {

	// OpenGL 着色器资源声明（如纹理、采样器等）
	class OpenGLShaderResourceDeclaration : public ShaderResourceDeclaration
	{
	public:
		// 资源类型枚举
		enum class Type
		{
			NONE, TEXTURE2D, TEXTURECUBE
		};
	private:
		friend class OpenGLShader;
	private:
		std::string m_Name;      // 资源名称
		uint32_t m_Register = 0; // 绑定槽
		uint32_t m_Count;        // 资源数量
		Type m_Type;             // 资源类型
	public:
		// 构造函数
		OpenGLShaderResourceDeclaration(Type type, const std::string& name, uint32_t count);


		inline const std::string& GetName() const override { return m_Name; }	// 获取资源名称
		inline uint32_t GetRegister() const override { return m_Register; }		// 获取绑定槽
		inline uint32_t GetCount() const override { return m_Count; }		    // 获取资源数量

		inline Type GetType() const { return m_Type; }		// 获取资源类型
	public:
		// 字符串与类型的转换
		static Type StringToType(const std::string& type);
		static std::string TypeToString(Type type);
	};

	// OpenGL 着色器 Uniform 变量声明
	class OpenGLShaderUniformDeclaration : public ShaderUniformDeclaration
	{
	private:
		friend class OpenGLShader;
		friend class OpenGLShaderUniformBufferDeclaration;
	public:
		// Uniform 类型枚举
		enum class Type
		{
			NONE, FLOAT32, VEC2, VEC3, VEC4, MAT3, MAT4, INT32, STRUCT
		};
	private:
		std::string m_Name;			// 变量名
		uint32_t m_Size;			// 变量大小
		uint32_t m_Count;			// 数组长度
		uint32_t m_Offset;			// 在缓冲区中的偏移
		ShaderDomain m_Domain;		// 所属着色器域

		Type m_Type;				// 变量类型
		ShaderStruct* m_Struct;		// 结构体类型（如为结构体）
		mutable int32_t m_Location; // OpenGL 变量位置
	public:
		OpenGLShaderUniformDeclaration(ShaderDomain domain, Type type, const std::string& name, uint32_t count = 1);
		OpenGLShaderUniformDeclaration(ShaderDomain domain, ShaderStruct* uniformStruct, const std::string& name, uint32_t count = 1);

		inline const std::string& GetName() const override { return m_Name; }		// 获取变量名
		inline uint32_t GetSize() const override { return m_Size; }					// 获取变量大小
		inline uint32_t GetCount() const override { return m_Count; }				// 获取数组长度
		inline uint32_t GetOffset() const override { return m_Offset; }				// 获取偏移
		inline uint32_t GetAbsoluteOffset() const { return m_Struct ? m_Struct->GetOffset() + m_Offset : m_Offset; }		// 获取绝对偏移（结构体内偏移）
		inline ShaderDomain GetDomain() const { return m_Domain; }					// 获取所属着色器域

		int32_t GetLocation() const { return m_Location; }		// 获取 OpenGL 变量位置
		inline Type GetType() const { return m_Type; }			// 获取变量类型
		inline bool IsArray() const { return m_Count > 1; }		// 是否为数组

		inline const ShaderStruct& GetShaderUniformStruct() const { HZ_CORE_ASSERT(m_Struct, ""); return *m_Struct; }		// 获取结构体类型（断言必须为结构体）
	protected:
		// 设置偏移（仅限内部使用）
		void SetOffset(uint32_t offset) override;
	public:
		// 类型与字符串的转换、类型大小
		static uint32_t SizeOfUniformType(Type type);
		static Type StringToType(const std::string& type);
		static std::string TypeToString(Type type);
	};

	// OpenGL Uniform 字段信息结构体
	struct GLShaderUniformField
	{
		OpenGLShaderUniformDeclaration::Type type; // 字段类型
		std::string name;                          // 字段名
		uint32_t count;                            // 数组长度
		mutable uint32_t size;                     // 字段大小
		mutable int32_t location;                  // OpenGL 变量位置
	};

	// OpenGL Uniform 缓冲区声明
	class OpenGLShaderUniformBufferDeclaration : public ShaderUniformBufferDeclaration
	{
	private:
		friend class Shader;
	private:
		std::string m_Name;            // 缓冲区名称
		ShaderUniformList m_Uniforms;  // Uniform 列表
		uint32_t m_Register;           // 绑定槽
		uint32_t m_Size;               // 缓冲区大小
		ShaderDomain m_Domain;         // 所属着色器域
	public:
		OpenGLShaderUniformBufferDeclaration(const std::string& name, ShaderDomain domain);

		void PushUniform(OpenGLShaderUniformDeclaration* uniform);				// 添加 Uniform 声明

		inline const std::string& GetName() const override { return m_Name; }	// 获取缓冲区名称
		inline uint32_t GetRegister() const override { return m_Register; }		// 获取绑定槽
		inline uint32_t GetSize() const override { return m_Size; }				// 获取缓冲区大小
		virtual ShaderDomain GetDomain() const { return m_Domain; }				// 获取所属着色器域
		inline const ShaderUniformList& GetUniformDeclarations() const override { return m_Uniforms; }		// 获取所有 Uniform 声明

		ShaderUniformDeclaration* FindUniform(const std::string& name);			// 查找指定名称的 Uniform
	};

}
