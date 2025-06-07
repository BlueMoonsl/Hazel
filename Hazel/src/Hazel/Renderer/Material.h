#pragma once

#include "Hazel/Core/Base.h"

#include "Hazel/Renderer/Shader.h"
#include "Hazel/Renderer/Texture.h"

#include <unordered_set>

namespace Hazel {

	// 材质基类
	class Material
	{
		friend class MaterialInstance;
	public:
		Material(const Ref<Shader>& shader);
		virtual ~Material();

		void Bind() const;

		// 设置通用类型的 Uniform 变量
		template <typename T>
		void Set(const std::string& name, const T& value)
		{
			auto decl = FindUniformDeclaration(name);
			// HZ_CORE_ASSERT(decl, "Could not find uniform with name '{0}'", name);
			HZ_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
			auto& buffer = GetUniformBufferTarget(decl);
			buffer.Write((byte*)&value, decl->GetSize(), decl->GetOffset());

			// 通知所有实例更新
			for (auto mi : m_MaterialInstances)
				mi->OnMaterialValueUpdated(decl);
		}

		// 设置通用纹理
		void Set(const std::string& name, const Ref<Texture>& texture)
		{
			auto decl = FindResourceDeclaration(name);
			uint32_t slot = decl->GetRegister();
			if (m_Textures.size() <= slot)
				m_Textures.resize((size_t)slot + 1);
			m_Textures[slot] = texture;
		}

		// 设置2D纹理
		void Set(const std::string& name, const Ref<Texture2D>& texture)
		{
			Set(name, (const Ref<Texture>&)texture);
		}

		// 设置立方体纹理
		void Set(const std::string& name, const Ref<TextureCube>& texture)
		{
			Set(name, (const Ref<Texture>&)texture);
		}
	public:
		static Ref<Material> Create(const Ref<Shader>& shader);
	private:
		void AllocateStorage();			// 分配 Uniform 存储空间
		void OnShaderReloaded();		// 着色器重新加载时的回调
		void BindTextures() const;		// 绑定所有纹理

		ShaderUniformDeclaration* FindUniformDeclaration(const std::string& name);		// 查找 Uniform 声明
		ShaderResourceDeclaration* FindResourceDeclaration(const std::string& name);	// 查找资源声明（如纹理）
		Buffer& GetUniformBufferTarget(ShaderUniformDeclaration* uniformDeclaration);	// 获取 Uniform 缓冲区目标
	private:
		Ref<Shader> m_Shader;			 // 着色器引用
		std::unordered_set<MaterialInstance*> m_MaterialInstances; // 该材质的所有实例

		Buffer m_VSUniformStorageBuffer; // 顶点着色器 Uniform 缓冲区
		Buffer m_PSUniformStorageBuffer; // 像素着色器 Uniform 缓冲区
		std::vector<Ref<Texture>> m_Textures; // 材质绑定的纹理

		int32_t m_RenderFlags = 0; // 渲染标志
	};

	// 材质实例类，允许对单个实例进行参数覆盖
	class MaterialInstance
	{
		friend class Material;
	public:
		// 构造函数，传入材质引用
		MaterialInstance(const Ref<Material>& material);
		virtual ~MaterialInstance();

		// 设置通用类型的 Uniform 变量（可覆盖）
		template <typename T>
		void Set(const std::string& name, const T& value)
		{
			auto decl = m_Material->FindUniformDeclaration(name);
			if (!decl)
				return;
			// HZ_CORE_ASSERT(decl, "Could not find uniform with name '{0}'", name);
			HZ_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
			auto& buffer = GetUniformBufferTarget(decl);
			buffer.Write((byte*)&value, decl->GetSize(), decl->GetOffset());

			m_OverriddenValues.insert(name);
		}

		// 设置通用纹理
		void Set(const std::string& name, const Ref<Texture>& texture)
		{
			auto decl = m_Material->FindResourceDeclaration(name);
			uint32_t slot = decl->GetRegister();
			if (m_Textures.size() <= slot)
				m_Textures.resize((size_t)slot + 1);
			m_Textures[slot] = texture;
		}

		// 设置2D纹理
		void Set(const std::string& name, const Ref<Texture2D>& texture)
		{
			Set(name, (const Ref<Texture>&)texture);
		}

		// 设置立方体纹理
		void Set(const std::string& name, const Ref<TextureCube>& texture)
		{
			Set(name, (const Ref<Texture>&)texture);
		}

		// 绑定材质实例
		void Bind() const;
	public:
		static Ref<MaterialInstance> Create(const Ref<Material>& material);
	private:
		void AllocateStorage();			 // 分配 Uniform 存储空间
		void OnShaderReloaded();		 // 着色器重新加载时的回调
		Buffer& GetUniformBufferTarget(ShaderUniformDeclaration* uniformDeclaration);	// 获取 Uniform 缓冲区目标
		void OnMaterialValueUpdated(ShaderUniformDeclaration* decl);					// 材质值更新时的回调
	private:
		Ref<Material> m_Material;		 // 所属材质

		Buffer m_VSUniformStorageBuffer; // 顶点着色器 Uniform 缓冲区
		Buffer m_PSUniformStorageBuffer; // 像素着色器 Uniform 缓冲区
		std::vector<Ref<Texture>> m_Textures; // 绑定的纹理

		// TODO: 临时方案，用于追踪被覆盖的参数
		std::unordered_set<std::string> m_OverriddenValues;
	};

}
