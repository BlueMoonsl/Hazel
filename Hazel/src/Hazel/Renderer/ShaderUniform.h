#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Core/Log.h"

#include <string>
#include <vector>

namespace Hazel {

	// ��ɫ�����������򣨶���/������ɫ����
	enum class ShaderDomain
	{
		None = 0, Vertex = 0, Pixel = 1
	};

	// ��ɫ�� Uniform ���������ĳ������
	class ShaderUniformDeclaration
	{
	private:
		friend class Shader;
		friend class OpenGLShader;
		friend class ShaderStruct;
	public:
		virtual const std::string& GetName() const = 0;      // ��ȡ������
		virtual uint32_t GetSize() const = 0;                // ��ȡ������С���ֽڣ�
		virtual uint32_t GetCount() const = 0;               // ��ȡ��������������֧�֣�
		virtual uint32_t GetOffset() const = 0;              // ��ȡ�����ڻ������е�ƫ��
		virtual ShaderDomain GetDomain() const = 0;          // ��ȡ����������ɫ����
	protected:
		virtual void SetOffset(uint32_t offset) = 0;         // ���ñ���ƫ�ƣ������ڲ�ʹ�ã�
	};

	typedef std::vector<ShaderUniformDeclaration*> ShaderUniformList;

	// Uniform �����������ĳ������
	class ShaderUniformBufferDeclaration
	{
	public:
		virtual const std::string& GetName() const = 0;                        // ��ȡ��������
		virtual uint32_t GetRegister() const = 0;                              // ��ȡ�������Ĵ�����
		virtual uint32_t GetSize() const = 0;                                  // ��ȡ�������ܴ�С
		virtual const ShaderUniformList& GetUniformDeclarations() const = 0;   // ��ȡ���� Uniform ����

		virtual ShaderUniformDeclaration* FindUniform(const std::string& name) = 0; // ����ָ�����Ƶ� Uniform
	};

	typedef std::vector<ShaderUniformBufferDeclaration*> ShaderUniformBufferList;

	// ��ɫ���ṹ�����������ڸ������� Uniform��
	class ShaderStruct
	{
	private:
		friend class Shader;
	private:
		std::string m_Name;                                 // �ṹ������
		std::vector<ShaderUniformDeclaration*> m_Fields;    // �ֶ��б�
		uint32_t m_Size;                                    // �ṹ���ܴ�С
		uint32_t m_Offset;                                  // �ṹ���ڻ������е�ƫ��
	public:
		ShaderStruct(const std::string& name)
			: m_Name(name), m_Size(0), m_Offset(0)
		{
		}

		// ����ֶΣ����Զ�����ƫ�ƺͽṹ���С
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

		inline void SetOffset(uint32_t offset) { m_Offset = offset; } // ���ýṹ��ƫ��

		inline const std::string& GetName() const { return m_Name; }
		inline uint32_t GetSize() const { return m_Size; }
		inline uint32_t GetOffset() const { return m_Offset; }
		inline const std::vector<ShaderUniformDeclaration*>& GetFields() const { return m_Fields; }
	};

	typedef std::vector<ShaderStruct*> ShaderStructList;

	// ��ɫ����Դ�������������������ȣ��ĳ������
	class ShaderResourceDeclaration
	{
	public:
		virtual const std::string& GetName() const = 0;     // ��ȡ��Դ��
		virtual uint32_t GetRegister() const = 0;           // ��ȡ��Դ�󶨲�
		virtual uint32_t GetCount() const = 0;              // ��ȡ��Դ����
	};

	typedef std::vector<ShaderResourceDeclaration*> ShaderResourceList;

}
