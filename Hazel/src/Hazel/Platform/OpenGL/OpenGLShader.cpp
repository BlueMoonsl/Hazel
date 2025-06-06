#include "hzpch.h"
#include "OpenGLShader.h"

#include <string>
#include <sstream>
#include <limits>

#include <glm/gtc/type_ptr.hpp>

#include "Hazel/Renderer/Renderer.h"

namespace Hazel {

#define UNIFORM_LOGGING 0
#if UNIFORM_LOGGING
#define HZ_LOG_UNIFORM(...) HZ_CORE_WARN(__VA_ARGS__)
#else
#define HZ_LOG_UNIFORM
#endif

	// 构造函数：根据文件路径初始化 OpenGLShader，并自动加载
	OpenGLShader::OpenGLShader(const std::string& filepath)
		: m_AssetPath(filepath)
	{
		size_t found = filepath.find_last_of("/\\");
		m_Name = found != std::string::npos ? filepath.substr(found + 1) : filepath;
		found = m_Name.find_last_of(".");
		m_Name = found != std::string::npos ? m_Name.substr(0, found) : m_Name;
		
		Reload();
	}

	Ref<OpenGLShader> OpenGLShader::CreateFromString(const std::string& source)
	{
		Ref<OpenGLShader> shader = std::make_shared<OpenGLShader>();
		shader->Load(source);
		return shader;
	}

	// 重新加载着色器（读取源码、编译、解析 Uniform 等）
	void OpenGLShader::Reload()
	{
		std::string source = ReadShaderFromFile(m_AssetPath);
		Load(source);
	}

	void OpenGLShader::Load(const std::string& source)
	{
		m_ShaderSource = PreProcess(source);
		if (!m_IsCompute)
			Parse();

		Renderer::Submit([this]()
		{
			if (m_RendererID)
				glDeleteShader(m_RendererID);

			CompileAndUploadShader();
			if (!m_IsCompute)
			{
				ResolveUniforms();
				ValidateUniforms();
			}

			if (m_Loaded)
			{
				for (auto& callback : m_ShaderReloadedCallbacks)
					callback();
			}

			m_Loaded = true;
		});
	}

	// 添加着色器重新加载回调
	void OpenGLShader::AddShaderReloadedCallback(const ShaderReloadedCallback& callback)
	{
		m_ShaderReloadedCallbacks.push_back(callback);
	}

	// 绑定着色器到渲染管线
	void OpenGLShader::Bind()
	{
		Renderer::Submit([=]() {
			glUseProgram(m_RendererID);
		});
	}

	// 从文件读取着色器源码
	std::string OpenGLShader::ReadShaderFromFile(const std::string& filepath) const
	{
		std::string result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary);
		if (in)
		{
			in.seekg(0, std::ios::end);
			result.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(&result[0], result.size());
			in.close();
		}
		else
		{
			HZ_CORE_ASSERT(false, "Could not load shader!");
		}

		return result;
	}

	// 预处理源码，按 #type 分割不同着色器类型
	std::unordered_map<GLenum, std::string> OpenGLShader::PreProcess(const std::string& source)
	{
		std::unordered_map<GLenum, std::string> shaderSources;

		const char* typeToken = "#type";
		size_t typeTokenLength = strlen(typeToken);
		size_t pos = source.find(typeToken, 0);
		while (pos != std::string::npos)
		{
			size_t eol = source.find_first_of("\r\n", pos);
			HZ_CORE_ASSERT(eol != std::string::npos, "Syntax error");
			size_t begin = pos + typeTokenLength + 1;
			std::string type = source.substr(begin, eol - begin);
			HZ_CORE_ASSERT(type == "vertex" || type == "fragment" || type == "pixel" || type == "compute", "Invalid shader type specified");

			size_t nextLinePos = source.find_first_not_of("\r\n", eol);
			pos = source.find(typeToken, nextLinePos);
			auto shaderType = ShaderTypeFromString(type);
				shaderSources[shaderType] = source.substr(nextLinePos, pos - (nextLinePos == std::string::npos ? source.size() - 1 : nextLinePos));

			// Compute shaders cannot contain other types
			if (shaderType == GL_COMPUTE_SHADER)
			{
				m_IsCompute = true;
				break;
			}
		}

		return shaderSources;
	}

	// ===================== 解析辅助函数 =====================

	// 查找 token（如 "struct"、"uniform"）在字符串中的位置
	const char* FindToken(const char* str, const std::string& token)
	{
		const char* t = str;
		while (t = strstr(t, token.c_str()))
		{
			bool left = str == t || isspace(t[-1]);
			bool right = !t[token.size()] || isspace(t[token.size()]);
			if (left && right)
				return t;

			t += token.size();
		}
		return nullptr;
	}

	const char* FindToken(const std::string& string, const std::string& token)
	{
		return FindToken(string.c_str(), token);
	}

	// 按分隔符分割字符串
	std::vector<std::string> SplitString(const std::string& string, const std::string& delimiters)
	{
		size_t start = 0;
		size_t end = string.find_first_of(delimiters);

		std::vector<std::string> result;

		while (end <= std::string::npos)
		{
			std::string token = string.substr(start, end - start);
			if (!token.empty())
				result.push_back(token);

			if (end == std::string::npos)
				break;

			start = end + 1;
			end = string.find_first_of(delimiters, start);
		}

		return result;
	}

	std::vector<std::string> SplitString(const std::string& string, const char delimiter)
	{
		return SplitString(string, std::string(1, delimiter));
	}

	// 按空白符分词
	std::vector<std::string> Tokenize(const std::string& string)
	{
		return SplitString(string, " \t\n");
	}

	// 按行分割
	std::vector<std::string> GetLines(const std::string& string)
	{
		return SplitString(string, "\n");
	}

	// 获取大括号包围的代码块
	std::string GetBlock(const char* str, const char** outPosition)
	{
		const char* end = strstr(str, "}");
		if (!end)
			return str;

		if (outPosition)
			*outPosition = end;
		uint32_t length = end - str + 1;
		return std::string(str, length);
	}

	// 获取分号结尾的语句
	std::string GetStatement(const char* str, const char** outPosition)
	{
		const char* end = strstr(str, ";");
		if (!end)
			return str;

		if (outPosition)
			*outPosition = end;
		uint32_t length = end - str + 1;
		return std::string(str, length);
	}

	// 判断字符串是否以指定前缀开头
	bool StartsWith(const std::string& string, const std::string& start)
	{
		return string.find(start) == 0;
	}

	// ===================== 着色器源码解析 =====================

	// 解析着色器源码，提取 struct 和 uniform 声明
	void OpenGLShader::Parse()
	{
		const char* token;
		const char* vstr;
		const char* fstr;

		m_Resources.clear();
		m_Structs.clear();
		m_VSMaterialUniformBuffer.reset();
		m_PSMaterialUniformBuffer.reset();

		auto& vertexSource = m_ShaderSource[GL_VERTEX_SHADER];
		auto& fragmentSource = m_ShaderSource[GL_FRAGMENT_SHADER];

		// 解析顶点着色器 struct
		vstr = vertexSource.c_str();
		while (token = FindToken(vstr, "struct"))
			ParseUniformStruct(GetBlock(token, &vstr), ShaderDomain::Vertex);

		// 解析顶点着色器 uniform
		vstr = vertexSource.c_str();
		while (token = FindToken(vstr, "uniform"))
			ParseUniform(GetStatement(token, &vstr), ShaderDomain::Vertex);

		// 解析片元着色器 struct
		fstr = fragmentSource.c_str();
		while (token = FindToken(fstr, "struct"))
			ParseUniformStruct(GetBlock(token, &fstr), ShaderDomain::Pixel);

		// 解析片元着色器 uniform
		fstr = fragmentSource.c_str();
		while (token = FindToken(fstr, "uniform"))
			ParseUniform(GetStatement(token, &fstr), ShaderDomain::Pixel);
	}

	// 判断类型字符串是否为资源类型（如采样器/纹理）
	static bool IsTypeStringResource(const std::string& type)
	{
		if (type == "sampler2D")		return true;
		if (type == "samplerCube")		return true;
		if (type == "sampler2DShadow")	return true;
		return false;
	}

	// 查找结构体声明
	ShaderStruct* OpenGLShader::FindStruct(const std::string& name)
	{
		for (ShaderStruct* s : m_Structs)
		{
			if (s->GetName() == name)
				return s;
		}
		return nullptr;
	}

	// 解析 uniform 声明
	void OpenGLShader::ParseUniform(const std::string& statement, ShaderDomain domain)
	{
		std::vector<std::string> tokens = Tokenize(statement);
		uint32_t index = 0;

		index++; // "uniform"
		std::string typeString = tokens[index++];
		std::string name = tokens[index++];
		// 去除变量名末尾的分号
		if (const char* s = strstr(name.c_str(), ";"))
			name = std::string(name.c_str(), s - name.c_str());

		std::string n(name);
		int32_t count = 1;
		const char* namestr = n.c_str();
		if (const char* s = strstr(namestr, "["))
		{
			name = std::string(namestr, s - namestr);

			const char* end = strstr(namestr, "]");
			std::string c(s + 1, end - s);
			count = atoi(c.c_str());
		}

		if (IsTypeStringResource(typeString))
		{
			// 资源类型 uniform
			ShaderResourceDeclaration* declaration = new OpenGLShaderResourceDeclaration(OpenGLShaderResourceDeclaration::StringToType(typeString), name, count);
			m_Resources.push_back(declaration);
		}
		else
		{
			// 普通类型 uniform
			OpenGLShaderUniformDeclaration::Type t = OpenGLShaderUniformDeclaration::StringToType(typeString);
			OpenGLShaderUniformDeclaration* declaration = nullptr;

			if (t == OpenGLShaderUniformDeclaration::Type::NONE)
			{
				// 结构体类型 uniform
				ShaderStruct* s = FindStruct(typeString);
				HZ_CORE_ASSERT(s, "");
				declaration = new OpenGLShaderUniformDeclaration(domain, s, name, count);
			}
			else
			{
				declaration = new OpenGLShaderUniformDeclaration(domain, t, name, count);
			}

			// 以 r_ 开头的 uniform 归为渲染器 uniform，否则归为材质 uniform
			if (StartsWith(name, "r_"))
			{
				if (domain == ShaderDomain::Vertex)
					((OpenGLShaderUniformBufferDeclaration*)m_VSRendererUniformBuffers.front())->PushUniform(declaration);
				else if (domain == ShaderDomain::Pixel)
					((OpenGLShaderUniformBufferDeclaration*)m_PSRendererUniformBuffers.front())->PushUniform(declaration);
			}
			else
			{
				if (domain == ShaderDomain::Vertex)
				{
					if (!m_VSMaterialUniformBuffer)
						m_VSMaterialUniformBuffer.reset(new OpenGLShaderUniformBufferDeclaration("", domain));

					m_VSMaterialUniformBuffer->PushUniform(declaration);
				}
				else if (domain == ShaderDomain::Pixel)
				{
					if (!m_PSMaterialUniformBuffer)
						m_PSMaterialUniformBuffer.reset(new OpenGLShaderUniformBufferDeclaration("", domain));

					m_PSMaterialUniformBuffer->PushUniform(declaration);
				}
			}
		}
	}

	// 解析 struct 声明
	void OpenGLShader::ParseUniformStruct(const std::string& block, ShaderDomain domain)
	{
		std::vector<std::string> tokens = Tokenize(block);

		uint32_t index = 0;
		index++; // struct
		std::string name = tokens[index++];
		ShaderStruct* uniformStruct = new ShaderStruct(name);
		index++; // {
		while (index < tokens.size())
		{
			if (tokens[index] == "}")
				break;

			std::string type = tokens[index++];
			std::string name = tokens[index++];

			// 去除变量名末尾的分号
			if (const char* s = strstr(name.c_str(), ";"))
				name = std::string(name.c_str(), s - name.c_str());

			uint32_t count = 1;
			const char* namestr = name.c_str();
			if (const char* s = strstr(namestr, "["))
			{
				name = std::string(namestr, s - namestr);

				const char* end = strstr(namestr, "]");
				std::string c(s + 1, end - s);
				count = atoi(c.c_str());
			}
			ShaderUniformDeclaration* field = new OpenGLShaderUniformDeclaration(domain, OpenGLShaderUniformDeclaration::StringToType(type), name, count);
			uniformStruct->AddField(field);
		}
		m_Structs.push_back(uniformStruct);
	}

	// 解析并缓存所有 Uniform 的 location
	void OpenGLShader::ResolveUniforms()
	{
		glUseProgram(m_RendererID);

		// 解析顶点着色器渲染器 Uniform
		for (size_t i = 0; i < m_VSRendererUniformBuffers.size(); i++)
		{
			OpenGLShaderUniformBufferDeclaration* decl = (OpenGLShaderUniformBufferDeclaration*)m_VSRendererUniformBuffers[i];
			const ShaderUniformList& uniforms = decl->GetUniformDeclarations();
			for (size_t j = 0; j < uniforms.size(); j++)
			{
				OpenGLShaderUniformDeclaration* uniform = (OpenGLShaderUniformDeclaration*)uniforms[j];
				if (uniform->GetType() == OpenGLShaderUniformDeclaration::Type::STRUCT)
				{
					const ShaderStruct& s = uniform->GetShaderUniformStruct();
					const auto& fields = s.GetFields();
					for (size_t k = 0; k < fields.size(); k++)
					{
						OpenGLShaderUniformDeclaration* field = (OpenGLShaderUniformDeclaration*)fields[k];
						field->m_Location = GetUniformLocation(uniform->m_Name + "." + field->m_Name);
					}
				}
				else
				{
					uniform->m_Location = GetUniformLocation(uniform->m_Name);
				}
			}
		}

		// 解析片元着色器渲染器 Uniform
		for (size_t i = 0; i < m_PSRendererUniformBuffers.size(); i++)
		{
			OpenGLShaderUniformBufferDeclaration* decl = (OpenGLShaderUniformBufferDeclaration*)m_PSRendererUniformBuffers[i];
			const ShaderUniformList& uniforms = decl->GetUniformDeclarations();
			for (size_t j = 0; j < uniforms.size(); j++)
			{
				OpenGLShaderUniformDeclaration* uniform = (OpenGLShaderUniformDeclaration*)uniforms[j];
				if (uniform->GetType() == OpenGLShaderUniformDeclaration::Type::STRUCT)
				{
					const ShaderStruct& s = uniform->GetShaderUniformStruct();
					const auto& fields = s.GetFields();
					for (size_t k = 0; k < fields.size(); k++)
					{
						OpenGLShaderUniformDeclaration* field = (OpenGLShaderUniformDeclaration*)fields[k];
						field->m_Location = GetUniformLocation(uniform->m_Name + "." + field->m_Name);
					}
				}
				else
				{
					uniform->m_Location = GetUniformLocation(uniform->m_Name);
				}
			}
		}

		// 解析顶点材质 Uniform
		{
			const auto& decl = m_VSMaterialUniformBuffer;
			if (decl)
			{
				const ShaderUniformList& uniforms = decl->GetUniformDeclarations();
				for (size_t j = 0; j < uniforms.size(); j++)
				{
					OpenGLShaderUniformDeclaration* uniform = (OpenGLShaderUniformDeclaration*)uniforms[j];
					if (uniform->GetType() == OpenGLShaderUniformDeclaration::Type::STRUCT)
					{
						const ShaderStruct& s = uniform->GetShaderUniformStruct();
						const auto& fields = s.GetFields();
						for (size_t k = 0; k < fields.size(); k++)
						{
							OpenGLShaderUniformDeclaration* field = (OpenGLShaderUniformDeclaration*)fields[k];
							field->m_Location = GetUniformLocation(uniform->m_Name + "." + field->m_Name);
						}
					}
					else
					{
						uniform->m_Location = GetUniformLocation(uniform->m_Name);
					}
				}
			}
		}

		// 解析片元材质 Uniform
		{
			const auto& decl = m_PSMaterialUniformBuffer;
			if (decl)
			{
				const ShaderUniformList& uniforms = decl->GetUniformDeclarations();
				for (size_t j = 0; j < uniforms.size(); j++)
				{
					OpenGLShaderUniformDeclaration* uniform = (OpenGLShaderUniformDeclaration*)uniforms[j];
					if (uniform->GetType() == OpenGLShaderUniformDeclaration::Type::STRUCT)
					{
						const ShaderStruct& s = uniform->GetShaderUniformStruct();
						const auto& fields = s.GetFields();
						for (size_t k = 0; k < fields.size(); k++)
						{
							OpenGLShaderUniformDeclaration* field = (OpenGLShaderUniformDeclaration*)fields[k];
							field->m_Location = GetUniformLocation(uniform->m_Name + "." + field->m_Name);
						}
					}
					else
					{
						uniform->m_Location = GetUniformLocation(uniform->m_Name);
					}
				}
			}
		}

		// 解析所有资源 Uniform（如采样器/纹理），并分配绑定槽
		uint32_t sampler = 0;
		for (size_t i = 0; i < m_Resources.size(); i++)
		{
			OpenGLShaderResourceDeclaration* resource = (OpenGLShaderResourceDeclaration*)m_Resources[i];
			int32_t location = GetUniformLocation(resource->m_Name);

			if (resource->GetCount() == 1)
			{
				resource->m_Register = sampler;
				if (location != -1)
					UploadUniformInt(location, sampler);

				sampler++;
			}
			else if (resource->GetCount() > 1)
			{
				resource->m_Register = 0;
				uint32_t count = resource->GetCount();
				int* samplers = new int[count];
				for (uint32_t s = 0; s < count; s++)
					samplers[s] = s;
				UploadUniformIntArray(resource->GetName(), samplers, count);
				delete[] samplers;
			}
		}
	}

	// 校验 Uniform（暂未实现）
	void OpenGLShader::ValidateUniforms()
	{

	}

	// 获取 Uniform 变量在 OpenGL 程序中的 location
	int32_t OpenGLShader::GetUniformLocation(const std::string& name) const
	{
		int32_t result = glGetUniformLocation(m_RendererID, name.c_str());
		if (result == -1)
			HZ_CORE_WARN("Could not find uniform '{0}' in shader", name);

		return result;
	}

	// 字符串转 OpenGL 着色器类型
	GLenum OpenGLShader::ShaderTypeFromString(const std::string& type)
	{
		if (type == "vertex")
			return GL_VERTEX_SHADER;
		if (type == "fragment" || type == "pixel")
			return GL_FRAGMENT_SHADER;
		if (type == "compute")
			return GL_COMPUTE_SHADER;

		return GL_NONE;
	}

	// 编译并上传着色器到 GPU
	void OpenGLShader::CompileAndUploadShader()
	{
		std::vector<GLuint> shaderRendererIDs;

		GLuint program = glCreateProgram();
		for (auto& kv : m_ShaderSource)
		{
			GLenum type = kv.first;
			std::string& source = kv.second;

			GLuint shaderRendererID = glCreateShader(type);
			const GLchar* sourceCstr = (const GLchar*)source.c_str();
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

				// 删除失败的 shader
				glDeleteShader(shaderRendererID);

				HZ_CORE_ASSERT(false, "Failed");
			}

			shaderRendererIDs.push_back(shaderRendererID);
			glAttachShader(program, shaderRendererID);
		}

		// 链接 shader program
		glLinkProgram(program);

		GLint isLinked = 0;
		glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);
		if (isLinked == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			// 获取链接错误日志
			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);
			HZ_CORE_ERROR("Shader compilation failed:\n{0}", &infoLog[0]);

			// 删除失败的 program
			glDeleteProgram(program);
			// 删除所有 shader
			for (auto id : shaderRendererIDs)
				glDeleteShader(id);
		}

		// 分离所有 shader
		for (auto id : shaderRendererIDs)
			glDetachShader(program, id);

		m_RendererID = program;
	}

	// 设置顶点着色器材质 Uniform 缓冲区
	void OpenGLShader::SetVSMaterialUniformBuffer(Buffer buffer)
	{
		Renderer::Submit([this, buffer]() {
			glUseProgram(m_RendererID);
			ResolveAndSetUniforms(m_VSMaterialUniformBuffer, buffer);
		});
	}

	// 设置片元着色器材质 Uniform 缓冲区
	void OpenGLShader::SetPSMaterialUniformBuffer(Buffer buffer)
	{
		Renderer::Submit([this, buffer]() {
			glUseProgram(m_RendererID);
			ResolveAndSetUniforms(m_PSMaterialUniformBuffer, buffer);
		});
	}

	// 解析并设置 Uniform 缓冲区中的所有 Uniform
	void OpenGLShader::ResolveAndSetUniforms(const Scope<OpenGLShaderUniformBufferDeclaration>& decl, Buffer buffer)
	{
		const ShaderUniformList& uniforms = decl->GetUniformDeclarations();
		for (size_t i = 0; i < uniforms.size(); i++)
		{
			OpenGLShaderUniformDeclaration* uniform = (OpenGLShaderUniformDeclaration*)uniforms[i];
			if (uniform->IsArray())
				ResolveAndSetUniformArray(uniform, buffer);
			else
				ResolveAndSetUniform(uniform, buffer);
		}
	}

	// 解析并设置单个 Uniform
	void OpenGLShader::ResolveAndSetUniform(OpenGLShaderUniformDeclaration* uniform, Buffer buffer)
	{
		if (uniform->GetLocation() == -1)
			return;

		HZ_CORE_ASSERT(uniform->GetLocation() != -1, "Uniform has invalid location!");

		uint32_t offset = uniform->GetOffset();
		switch (uniform->GetType())
		{
		case OpenGLShaderUniformDeclaration::Type::FLOAT32:
			UploadUniformFloat(uniform->GetLocation(), *(float*)&buffer.Data[offset]);
			break;
		case OpenGLShaderUniformDeclaration::Type::INT32:
			UploadUniformInt(uniform->GetLocation(), *(int32_t*)&buffer.Data[offset]);
			break;
		case OpenGLShaderUniformDeclaration::Type::VEC2:
			UploadUniformFloat2(uniform->GetLocation(), *(glm::vec2*)&buffer.Data[offset]);
			break;
		case OpenGLShaderUniformDeclaration::Type::VEC3:
			UploadUniformFloat3(uniform->GetLocation(), *(glm::vec3*)&buffer.Data[offset]);
			break;
		case OpenGLShaderUniformDeclaration::Type::VEC4:
			UploadUniformFloat4(uniform->GetLocation(), *(glm::vec4*)&buffer.Data[offset]);
			break;
		case OpenGLShaderUniformDeclaration::Type::MAT3:
			UploadUniformMat3(uniform->GetLocation(), *(glm::mat3*)&buffer.Data[offset]);
			break;
		case OpenGLShaderUniformDeclaration::Type::MAT4:
			UploadUniformMat4(uniform->GetLocation(), *(glm::mat4*)&buffer.Data[offset]);
			break;
		case OpenGLShaderUniformDeclaration::Type::STRUCT:
			UploadUniformStruct(uniform, buffer.Data, offset);
			break;
		default:
			HZ_CORE_ASSERT(false, "Unknown uniform type!");
		}
	}

	// 解析并设置 Uniform 数组
	void OpenGLShader::ResolveAndSetUniformArray(OpenGLShaderUniformDeclaration* uniform, Buffer buffer)
	{
		//HZ_CORE_ASSERT(uniform->GetLocation() != -1, "Uniform has invalid location!");

		uint32_t offset = uniform->GetOffset();
		switch (uniform->GetType())
		{
		case OpenGLShaderUniformDeclaration::Type::FLOAT32:
			UploadUniformFloat(uniform->GetLocation(), *(float*)&buffer.Data[offset]);
			break;
		case OpenGLShaderUniformDeclaration::Type::INT32:
			UploadUniformInt(uniform->GetLocation(), *(int32_t*)&buffer.Data[offset]);
			break;
		case OpenGLShaderUniformDeclaration::Type::VEC2:
			UploadUniformFloat2(uniform->GetLocation(), *(glm::vec2*)&buffer.Data[offset]);
			break;
		case OpenGLShaderUniformDeclaration::Type::VEC3:
			UploadUniformFloat3(uniform->GetLocation(), *(glm::vec3*)&buffer.Data[offset]);
			break;
		case OpenGLShaderUniformDeclaration::Type::VEC4:
			UploadUniformFloat4(uniform->GetLocation(), *(glm::vec4*)&buffer.Data[offset]);
			break;
		case OpenGLShaderUniformDeclaration::Type::MAT3:
			UploadUniformMat3(uniform->GetLocation(), *(glm::mat3*)&buffer.Data[offset]);
			break;
		case OpenGLShaderUniformDeclaration::Type::MAT4:
			UploadUniformMat4Array(uniform->GetLocation(), *(glm::mat4*)&buffer.Data[offset], uniform->GetCount());
			break;
		case OpenGLShaderUniformDeclaration::Type::STRUCT:
			UploadUniformStruct(uniform, buffer.Data, offset);
			break;
		default:
			HZ_CORE_ASSERT(false, "Unknown uniform type!");
		}
	}

	// 解析并设置结构体字段
	void OpenGLShader::ResolveAndSetUniformField(const OpenGLShaderUniformDeclaration& field, byte* data, int32_t offset)
	{
		switch (field.GetType())
		{
		case OpenGLShaderUniformDeclaration::Type::FLOAT32:
			UploadUniformFloat(field.GetLocation(), *(float*)&data[offset]);
			break;
		case OpenGLShaderUniformDeclaration::Type::INT32:
			UploadUniformInt(field.GetLocation(), *(int32_t*)&data[offset]);
			break;
		case OpenGLShaderUniformDeclaration::Type::VEC2:
			UploadUniformFloat2(field.GetLocation(), *(glm::vec2*)&data[offset]);
			break;
		case OpenGLShaderUniformDeclaration::Type::VEC3:
			UploadUniformFloat3(field.GetLocation(), *(glm::vec3*)&data[offset]);
			break;
		case OpenGLShaderUniformDeclaration::Type::VEC4:
			UploadUniformFloat4(field.GetLocation(), *(glm::vec4*)&data[offset]);
			break;
		case OpenGLShaderUniformDeclaration::Type::MAT3:
			UploadUniformMat3(field.GetLocation(), *(glm::mat3*)&data[offset]);
			break;
		case OpenGLShaderUniformDeclaration::Type::MAT4:
			UploadUniformMat4(field.GetLocation(), *(glm::mat4*)&data[offset]);
			break;
		default:
			HZ_CORE_ASSERT(false, "Unknown uniform type!");
		}
	}

	// 上传 UniformBufferBase 到 GPU
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
					Renderer::Submit([=]() {
						UploadUniformFloat(name, value);
						});
				}
				case UniformType::Float3:
				{
					const std::string& name = decl.Name;
					glm::vec3& values = *(glm::vec3*)(uniformBuffer.GetBuffer() + decl.Offset);
					Renderer::Submit([=]() {
						UploadUniformFloat3(name, values);
						});
				}
				case UniformType::Float4:
				{
					const std::string& name = decl.Name;
					glm::vec4& values = *(glm::vec4*)(uniformBuffer.GetBuffer() + decl.Offset);
					Renderer::Submit([=]() {
						UploadUniformFloat4(name, values);
						});
				}
				case UniformType::Matrix4x4:
				{
					const std::string& name = decl.Name;
					glm::mat4& values = *(glm::mat4*)(uniformBuffer.GetBuffer() + decl.Offset);
					Renderer::Submit([=]() {
						UploadUniformMat4(name, values);
						});
				}
			}
		}
	}

	// 临时接口：设置 float 类型 Uniform
	void OpenGLShader::SetFloat(const std::string& name, float value)
	{
		Renderer::Submit([=]() {
			UploadUniformFloat(name, value);
			});
	}

	// 临时接口：设置 mat4 类型 Uniform
	void OpenGLShader::SetMat4(const std::string& name, const glm::mat4& value)
	{
		Renderer::Submit([=]() {
			UploadUniformMat4(name, value);
			});
	}

	// 渲染线程直接设置 mat4 类型 Uniform
	void OpenGLShader::SetMat4FromRenderThread(const std::string& name, const glm::mat4& value, bool bind)
	{
		if (bind)
		{
			UploadUniformMat4(name, value);
		}
		else
		{
			int location = glGetUniformLocation(m_RendererID, name.c_str());
			if (location != -1)
				UploadUniformMat4(location, value);
		}
	}

	// 上传 int 类型 Uniform（通过 location）
	void OpenGLShader::UploadUniformInt(uint32_t location, int32_t value)
	{
		glUniform1i(location, value);
	}

	// 上传 int 数组 Uniform（通过 location）
	void OpenGLShader::UploadUniformIntArray(uint32_t location, int32_t* values, int32_t count)
	{
		glUniform1iv(location, count, values);
	}

	// 上传 float 类型 Uniform（通过 location）
	void OpenGLShader::UploadUniformFloat(uint32_t location, float value)
	{
		glUniform1f(location, value);
	}

	// 上传 vec2 类型 Uniform（通过 location）
	void OpenGLShader::UploadUniformFloat2(uint32_t location, const glm::vec2& value)
	{
		glUniform2f(location, value.x, value.y);
	}

	// 上传 vec3 类型 Uniform（通过 location）
	void OpenGLShader::UploadUniformFloat3(uint32_t location, const glm::vec3& value)
	{
		glUniform3f(location, value.x, value.y, value.z);
	}

	// 上传 vec4 类型 Uniform（通过 location）
	void OpenGLShader::UploadUniformFloat4(uint32_t location, const glm::vec4& value)
	{
		glUniform4f(location, value.x, value.y, value.z, value.w);
	}

	// 上传 mat3 类型 Uniform（通过 location）
	void OpenGLShader::UploadUniformMat3(uint32_t location, const glm::mat3& value)
	{
		glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
	}

	// 上传 mat4 类型 Uniform（通过 location）
	void OpenGLShader::UploadUniformMat4(uint32_t location, const glm::mat4& value)
	{
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
	}

	// 上传 mat4 数组 Uniform（通过 location）
	void OpenGLShader::UploadUniformMat4Array(uint32_t location, const glm::mat4& values, uint32_t count)
	{
		glUniformMatrix4fv(location, count, GL_FALSE, glm::value_ptr(values));
	}

	// 上传结构体类型 Uniform
	void OpenGLShader::UploadUniformStruct(OpenGLShaderUniformDeclaration* uniform, byte* buffer, uint32_t offset)
	{
		const ShaderStruct& s = uniform->GetShaderUniformStruct();
		const auto& fields = s.GetFields();
		for (size_t k = 0; k < fields.size(); k++)
		{
			OpenGLShaderUniformDeclaration* field = (OpenGLShaderUniformDeclaration*)fields[k];
			ResolveAndSetUniformField(*field, buffer, offset);
			offset += field->m_Size;
		}
	}

	// 上传 int 类型 Uniform（通过变量名）
	void OpenGLShader::UploadUniformInt(const std::string& name, int32_t value)
	{
		int32_t location = GetUniformLocation(name);
		glUniform1i(location, value);
	}

	// 上传 int 数组 Uniform（通过变量名）
	void OpenGLShader::UploadUniformIntArray(const std::string& name, int32_t* values, int32_t count)
	{
		int32_t location = GetUniformLocation(name);
		glUniform1iv(location, count, values);
	}

	// 上传 float 类型 Uniform（通过变量名）
	void OpenGLShader::UploadUniformFloat(const std::string& name, float value)
	{
		glUseProgram(m_RendererID);
		auto location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location != -1)
			glUniform1f(location, value);
		else
			HZ_LOG_UNIFORM("Uniform '{0}' not found!", name);
	}

	// 上传 vec3 类型 Uniform（通过变量名）
	void OpenGLShader::UploadUniformFloat3(const std::string& name, const glm::vec3& values)
	{
		glUseProgram(m_RendererID);
		auto location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location != -1)
			glUniform3f(location, values.x, values.y, values.z);
		else
			HZ_LOG_UNIFORM("Uniform '{0}' not found!", name);
	}

	// 上传 vec4 类型 Uniform（通过变量名）
	void OpenGLShader::UploadUniformFloat4(const std::string& name, const glm::vec4& values)
	{
		glUseProgram(m_RendererID);
		auto location = glGetUniformLocation(m_RendererID, name.c_str());
		if (location != -1)
			glUniform4f(location, values.x, values.y, values.z, values.w);
		else
			HZ_LOG_UNIFORM("Uniform '{0}' not found!", name);
	}

	// 上传 mat4 类型 Uniform（通过变量名）
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