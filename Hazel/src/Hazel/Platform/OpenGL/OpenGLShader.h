#pragma once

#include "Hazel/Renderer/Shader.h"
#include <glad/glad.h>

#include "OpenGLShaderUniform.h"

namespace Hazel {

	// OpenGLShader 类，OpenGL 平台下的着色器实现
	class OpenGLShader : public Shader
	{
	public:
		// 构造函数，传入着色器文件路径
		OpenGLShader() = default;
		OpenGLShader(const std::string& filepath);
		static Ref<OpenGLShader> CreateFromString(const std::string& source);

		// 重新加载着色器、添加重载回调
		virtual void Reload() override;
		virtual void AddShaderReloadedCallback(const ShaderReloadedCallback& callback) override;

		// 绑定着色器到渲染管线
		virtual void Bind() override;
		virtual RendererID GetRendererID() const override { return m_RendererID; }

		// 上传 Uniform 缓冲区到 GPU
		virtual void UploadUniformBuffer(const UniformBufferBase& uniformBuffer) override;

		// 设置顶点/像素着色器材质 Uniform 缓冲区
		virtual void SetVSMaterialUniformBuffer(Buffer buffer) override;
		virtual void SetPSMaterialUniformBuffer(Buffer buffer) override;

		// 临时接口：设置 float、mat4 类型 Uniform
		virtual void SetFloat(const std::string& name, float value) override;
		virtual void SetInt(const std::string& name, int value) override;
		virtual void SetMat4(const std::string& name, const glm::mat4& value) override;
		virtual void SetMat4FromRenderThread(const std::string& name, const glm::mat4& value, bool bind = true) override;

		virtual void SetIntArray(const std::string& name, int* values, uint32_t size) override;

		// 获取着色器名称
		virtual const std::string& GetName() const override { return m_Name; }
	private:
		void Load(const std::string& source);

		std::string ReadShaderFromFile(const std::string& filepath) const;				// 从文件读取着色器源码
		std::unordered_map<GLenum, std::string> PreProcess(const std::string& source);	// 预处理源码，按类型分割
		void Parse();		// 解析着色器源码
		void ParseUniform(const std::string& statement, ShaderDomain domain);			// 解析 Uniform 声明
		void ParseUniformStruct(const std::string& block, ShaderDomain domain);			// 解析 Uniform 结构体声明
		ShaderStruct* FindStruct(const std::string& name);								// 查找结构体声明

		int32_t GetUniformLocation(const std::string& name) const;		// 获取 Uniform 变量位置

		void ResolveUniforms();											// 解析并缓存所有 Uniform
		void ValidateUniforms();										// 校验 Uniform 合法性
		void CompileAndUploadShader();									// 编译并上传着色器到 GPU
		static GLenum ShaderTypeFromString(const std::string& type);	// 字符串转 OpenGL 着色器类型

		// 解析并设置 Uniform 缓冲区中的所有 Uniform、单个Uniform、Uniform 数组、结构体字段等
		void ResolveAndSetUniforms(const Scope<OpenGLShaderUniformBufferDeclaration>& decl, Buffer buffer);
		void ResolveAndSetUniform(OpenGLShaderUniformDeclaration* uniform, Buffer buffer);
		void ResolveAndSetUniformArray(OpenGLShaderUniformDeclaration* uniform, Buffer buffer);
		void ResolveAndSetUniformField(const OpenGLShaderUniformDeclaration& field, byte* data, int32_t offset);

		// 上传不同类型的 Uniform 到指定 location
		void UploadUniformInt(uint32_t location, int32_t value);
		void UploadUniformIntArray(uint32_t location, int32_t* values, uint32_t count);
		void UploadUniformFloat(uint32_t location, float value);
		void UploadUniformFloat2(uint32_t location, const glm::vec2& value);
		void UploadUniformFloat3(uint32_t location, const glm::vec3& value);
		void UploadUniformFloat4(uint32_t location, const glm::vec4& value);
		void UploadUniformMat3(uint32_t location, const glm::mat3& values);
		void UploadUniformMat4(uint32_t location, const glm::mat4& values);
		void UploadUniformMat4Array(uint32_t location, const glm::mat4& values, uint32_t count);

		// 上传结构体类型 Uniform
		void UploadUniformStruct(OpenGLShaderUniformDeclaration* uniform, byte* buffer, uint32_t offset);

		// 上传不同类型的 Uniform（通过变量名）
		void UploadUniformInt(const std::string& name, int32_t value);
		void UploadUniformIntArray(const std::string& name, int32_t* values, int32_t count);

		void UploadUniformFloat(const std::string& name, float value);
		void UploadUniformFloat2(const std::string& name, const glm::vec2& value);
		void UploadUniformFloat3(const std::string& name, const glm::vec3& value);
		void UploadUniformFloat4(const std::string& name, const glm::vec4& value);

		void UploadUniformMat4(const std::string& name, const glm::mat4& value);

		virtual const ShaderUniformBufferList& GetVSRendererUniforms() const override { return m_VSRendererUniformBuffers; }
		virtual const ShaderUniformBufferList& GetPSRendererUniforms() const override { return m_PSRendererUniformBuffers; }
		virtual bool HasVSMaterialUniformBuffer() const override { return (bool)m_VSMaterialUniformBuffer; }
		virtual bool HasPSMaterialUniformBuffer() const override { return (bool)m_PSMaterialUniformBuffer; }
		virtual const ShaderUniformBufferDeclaration& GetVSMaterialUniformBuffer() const override { return *m_VSMaterialUniformBuffer; }
		virtual const ShaderUniformBufferDeclaration& GetPSMaterialUniformBuffer() const override { return *m_PSMaterialUniformBuffer; }
		virtual const ShaderResourceList& GetResources() const override { return m_Resources; }
	private:
		RendererID m_RendererID = 0;	// OpenGL 着色器对象 ID
		bool m_Loaded = false;			// 是否已加载
		bool m_IsCompute = false;

		std::string m_Name, m_AssetPath; // 着色器名称和资源路径
		std::unordered_map<GLenum, std::string> m_ShaderSource; // 按类型存储源码

		std::vector<ShaderReloadedCallback> m_ShaderReloadedCallbacks; // 重新加载回调列表

		ShaderUniformBufferList m_VSRendererUniformBuffers; // 顶点着色器 Uniform 缓冲区声明
		ShaderUniformBufferList m_PSRendererUniformBuffers; // 像素着色器 Uniform 缓冲区声明
		Scope<OpenGLShaderUniformBufferDeclaration> m_VSMaterialUniformBuffer; // 顶点材质 Uniform 缓冲区
		Scope<OpenGLShaderUniformBufferDeclaration> m_PSMaterialUniformBuffer; // 像素材质 Uniform 缓冲区
		ShaderResourceList m_Resources; // 着色器资源列表
		ShaderStructList m_Structs;     // 结构体声明列表
	};

}