#include "hzpch.h"
#include "OpenGLShader.h"

#include <string>
#include <sstream>
#include <limits>

namespace Hazel {

#define UNIFORM_LOGGING 0
#if UNIFORM_LOGGING
#define HZ_LOG_UNIFORM(...) HZ_CORE_WARN(__VA_ARGS__)
#else
#define HZ_LOG_UNIFORM
#endif

	// OpenGLShader 构造函数，根据文件路径初始化并自动加载着色器
	OpenGLShader::OpenGLShader(const std::string& filepath)
		: m_AssetPath(filepath)
	{
		size_t found = filepath.find_last_of("/\\");
		m_Name = found != std::string::npos ? filepath.substr(found + 1) : filepath;
		Reload();
	}

	// 重新加载着色器（从文件读取并编译上传）
	void OpenGLShader::Reload()
	{
		ReadShaderFromFile(m_AssetPath);
		HZ_RENDER_S({
			if (self->m_RendererID)
				glDeleteShader(self->m_RendererID);

			self->CompileAndUploadShader();
		});
	}

	// 绑定着色器到OpenGL渲染管线
	void OpenGLShader::Bind()
	{
		HZ_RENDER_S({
			glUseProgram(self->m_RendererID);
		});
	}

	// 从文件读取着色器源码
	void OpenGLShader::ReadShaderFromFile(const std::string& filepath)
	{
		std::ifstream in(filepath, std::ios::in | std::ios::binary);
		if (in)
		{
			in.seekg(0, std::ios::end);
			m_ShaderSource.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(&m_ShaderSource[0], m_ShaderSource.size());
			in.close();
		}
		else
		{
			HZ_CORE_WARN("Could not read shader file {0}", filepath);
		}
	}

	// 根据字符串返回OpenGL着色器类型
	GLenum OpenGLShader::ShaderTypeFromString(const std::string& type)
	{
		if (type == "vertex")
			return GL_VERTEX_SHADER;
		if (type == "fragment" || type == "pixel")
			return GL_FRAGMENT_SHADER;

		return GL_NONE;
	}

	// 编译并上传着色器源码到GPU，处理多种着色器类型
	void OpenGLShader::CompileAndUploadShader()
	{
		std::unordered_map<GLenum, std::string> shaderSources;

		// 解析着色器源码中的 #type 标记，分离不同类型的着色器代码
		const char* typeToken = "#type";
		size_t typeTokenLength = strlen(typeToken);
		size_t pos = m_ShaderSource.find(typeToken, 0);
		while (pos != std::string::npos)
		{
			size_t eol = m_ShaderSource.find_first_of("\r\n", pos);
			HZ_CORE_ASSERT(eol != std::string::npos, "Syntax error");
			size_t begin = pos + typeTokenLength + 1;
			std::string type = m_ShaderSource.substr(begin, eol - begin);
			HZ_CORE_ASSERT(type == "vertex" || type == "fragment" || type == "pixel", "Invalid shader type specified");

			size_t nextLinePos = m_ShaderSource.find_first_not_of("\r\n", eol);
			pos = m_ShaderSource.find(typeToken, nextLinePos);
			shaderSources[ShaderTypeFromString(type)] = m_ShaderSource.substr(nextLinePos, pos - (nextLinePos == std::string::npos ? m_ShaderSource.size() - 1 : nextLinePos));
		}

		std::vector<GLuint> shaderRendererIDs;

		GLuint program = glCreateProgram();
		// 编译每个着色器类型并附加到程序
		for (auto& kv : shaderSources)
		{
			GLenum type = kv.first;
			std::string& source = kv.second;

			GLuint shaderRendererID = glCreateShader(type);
			const GLchar *sourceCstr = (const GLchar *)source.c_str();
			glShaderSource(shaderRendererID, 1, &sourceCstr, 0);

			glCompileShader(shaderRendererID);

			GLint isCompiled = 0;
			glGetShaderiv(shaderRendererID, GL_COMPILE_STATUS, &isCompiled);
			if (isCompiled == GL_FALSE)
			{
				GLint maxLength = 0;
				glGetShaderiv(shaderRendererID, GL_INFO_LOG_LENGTH, &maxLength);

				// 获取编译错误日志
				std::vector<GLchar> infoLog(maxLength);
				glGetShaderInfoLog(shaderRendererID, maxLength, &maxLength, &infoLog[0]);

				HZ_CORE_ERROR("Shader compilation failed:\n{0}", &infoLog[0]);

				// 删除失败的着色器对象
				glDeleteShader(shaderRendererID);

				HZ_CORE_ASSERT(false, "Failed");
			}

			shaderRendererIDs.push_back(shaderRendererID);
			glAttachShader(program, shaderRendererID);
		}

		// 链接着色器程序
		glLinkProgram(program);

		// 检查链接状态
		GLint isLinked = 0;
		glGetProgramiv(program, GL_LINK_STATUS, (int *)&isLinked);
		if (isLinked == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			// 获取链接错误日志
			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);
			HZ_CORE_ERROR("Shader compilation failed:\n{0}", &infoLog[0]);

			// 删除程序对象和着色器对象
			glDeleteProgram(program);
			for (auto id : shaderRendererIDs)
				glDeleteShader(id);
		}

		// 分离着色器对象
		for (auto id : shaderRendererIDs)
			glDetachShader(program, id);

		m_RendererID = program;

		// 绑定常用纹理采样器的默认纹理单元
		UploadUniformInt("u_Texture", 0);

		// PBR相关纹理采样器
		UploadUniformInt("u_AlbedoTexture", 1);
		UploadUniformInt("u_NormalTexture", 2);
		UploadUniformInt("u_MetalnessTexture", 3);
		UploadUniformInt("u_RoughnessTexture", 4);

		UploadUniformInt("u_EnvRadianceTex", 10);
		UploadUniformInt("u_EnvIrradianceTex", 11);

		UploadUniformInt("u_BRDFLUTTexture", 15);
	}

	// 批量上传Uniform缓冲区中的所有Uniform到GPU
	void OpenGLShader::UploadUniformBuffer(const UniformBufferBase& uniformBuffer)
	{
		for (unsigned int i = 0; i < uniformBuffer.GetUniformCount(); i++)
		{
			const UniformDecl& decl = uniformBuffer.GetUniforms()[i];
			switch (decl.Type)
			{
				case UniformType::Float:
				{
					const std::string& name = decl.Name;
					float value = *(float*)(uniformBuffer.GetBuffer() + decl.Offset);
					HZ_RENDER_S2(name, value, {
						self->UploadUniformFloat(name, value);
					});
				}
				case UniformType::Float3:
				{
					const std::string & name = decl.Name;
					glm::vec3& values = *(glm::vec3*)(uniformBuffer.GetBuffer() + decl.Offset);
					HZ_RENDER_S2(name, values, {
						self->UploadUniformFloat3(name, values);
					});
				}
				case UniformType::Float4:
				{
					const std::string& name = decl.Name;
					glm::vec4& values = *(glm::vec4*)(uniformBuffer.GetBuffer() + decl.Offset);
					HZ_RENDER_S2(name, values, {
						self->UploadUniformFloat4(name, values);
					});
				}
				case UniformType::Matrix4x4:
				{
					const std::string & name = decl.Name;
					glm::mat4& values = *(glm::mat4*)(uniformBuffer.GetBuffer() + decl.Offset);
					HZ_RENDER_S2(name, values, {
						self->UploadUniformMat4(name, values);
					});
				}
			}
		}
	}

	// 设置float类型Uniform
	void OpenGLShader::SetFloat(const std::string& name, float value)
	{
		HZ_RENDER_S2(name, value, {
			self->UploadUniformFloat(name, value);
			});
	}

	// 设置mat4类型Uniform
	void OpenGLShader::SetMat4(const std::string& name, const glm::mat4& value)
	{
		HZ_RENDER_S2(name, value, {
			self->UploadUniformMat4(name, value);
			});
	}

	// 上传int类型Uniform到GPU
	void OpenGLShader::UploadUniformInt(const std::string& name, int value)
	{
		glUseProgram(m_RendererID);
		auto location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location != -1)
			glUniform1i(location, value);
		else
			HZ_LOG_UNIFORM("Uniform '{0}' not found!", name);
	}

	// 上传float类型Uniform到GPU
	void OpenGLShader::UploadUniformFloat(const std::string& name, float value)
	{
		glUseProgram(m_RendererID);
		auto location = glGetUniformLocation(m_RendererID, name.c_str());
			if (location != -1)
				glUniform1f(location, value);
			else
				HZ_LOG_UNIFORM("Uniform '{0}' not found!", name);
	}

	// 上传vec3类型Uniform到GPU
	void OpenGLShader::UploadUniformFloat3(const std::string& name, const glm::vec3& values)
	{
		glUseProgram(m_RendererID);
		auto location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location != -1)
			glUniform3f(location, values.x, values.y, values.z);
		else
			HZ_LOG_UNIFORM("Uniform '{0}' not found!", name);
	}

	// 上传vec4类型Uniform到GPU
	void OpenGLShader::UploadUniformFloat4(const std::string& name, const glm::vec4& values)
	{
		glUseProgram(m_RendererID);
		auto location = glGetUniformLocation(m_RendererID, name.c_str());
			if (location != -1)
				glUniform4f(location, values.x, values.y, values.z, values.w);
			else
				HZ_LOG_UNIFORM("Uniform '{0}' not found!", name);
	}

	// 上传mat4类型Uniform到GPU
	void OpenGLShader::UploadUniformMat4(const std::string& name, const glm::mat4& values)
	{
		glUseProgram(m_RendererID);
		auto location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location != -1)
			glUniformMatrix4fv(location, 1, GL_FALSE, (const float*)&values);
		else
			HZ_LOG_UNIFORM("Uniform '{0}' not found!", name);
	}

}