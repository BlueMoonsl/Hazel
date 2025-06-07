#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "Hazel/Core/Timestep.h"

#include "Hazel/Renderer/VertexArray.h"
#include "Hazel/Renderer/Buffer.h"
#include "Hazel/Renderer/Shader.h"
#include "Hazel/Renderer/Material.h"

struct aiNode;
struct aiAnimation;
struct aiNodeAnim;
struct aiScene;

namespace Assimp {
	class Importer;
}

namespace Hazel {

	// Mesh 顶点数据结构，包含蒙皮动画相关信息
	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec3 Tangent;
		glm::vec3 Binormal;
		glm::vec2 Texcoord;
	};

	struct AnimatedVertex
	{
		glm::vec3 Position;   // 顶点位置
		glm::vec3 Normal;     // 法线
		glm::vec3 Tangent;    // 切线
		glm::vec3 Binormal;   // 副切线
		glm::vec2 Texcoord;   // 纹理坐标

		uint32_t IDs[4] = { 0, 0, 0, 0 };      // 受影响的骨骼ID（最多4个）
		float Weights[4]{ 0.0f, 0.0f, 0.0f, 0.0f }; // 骨骼权重（最多4个）

		// 添加骨骼数据（最多支持4个骨骼影响）
		void AddBoneData(uint32_t BoneID, float Weight)
		{
			for (size_t i = 0; i < 4; i++)
			{
				if (Weights[i] == 0.0)
				{
					IDs[i] = BoneID;
					Weights[i] = Weight;
					return;
				}
			}
			// TODO: 保留最大权重的4个骨骼
			HZ_CORE_WARN("Vertex has more than four bones/weights affecting it, extra data will be discarded (BoneID={0}, Weight={1})", BoneID, Weight);
		}
	};

	static const int NumAttributes = 5;

	// 三角形索引结构体，每个三角形由3个顶点索引组成
	struct Index
	{
		uint32_t V1, V2, V3;
	};

	// 编译期断言，确保 Index 结构体大小等于3个 uint32_t
	static_assert(sizeof(Index) == 3 * sizeof(uint32_t));

	// 骨骼信息结构体，包含骨骼偏移矩阵和最终变换矩阵
	struct BoneInfo
	{
		glm::mat4 BoneOffset;           // 骨骼偏移矩阵（骨骼空间到模型空间的变换）
		glm::mat4 FinalTransformation;  // 骨骼最终变换（用于动画）
	};

	// 顶点骨骼数据结构体，存储每个顶点受哪些骨骼影响及其权重（最多4个）
	struct VertexBoneData
	{
		uint32_t IDs[4];    // 骨骼ID
		float Weights[4];   // 骨骼权重

		VertexBoneData()
		{
			memset(IDs, 0, sizeof(IDs));
			memset(Weights, 0, sizeof(Weights));
		};

		// 添加骨骼数据（最多支持4个骨骼影响）
		void AddBoneData(uint32_t BoneID, float Weight)
		{
			for (size_t i = 0; i < 4; i++)
			{
				if (Weights[i] == 0.0)
				{
					IDs[i] = BoneID;
					Weights[i] = Weight;
					return;
				}
			}
			// 理论上不会到这里，超出4个骨骼直接断言
			HZ_CORE_ASSERT(false, "Too many bones!");
		}
	};

	// 子网格（Submesh）结构体，描述一个独立渲染的网格片段
	class Submesh
	{
	public:
		uint32_t BaseVertex;     // 顶点起始索引
		uint32_t BaseIndex;      // 索引起始位置
		uint32_t MaterialIndex;  // 材质索引
		uint32_t IndexCount;     // 索引数量

		glm::mat4 Transform;
		glm::vec3 Min, Max; // TODO: AABB
	};

	// 网格（Mesh）类，支持 Assimp 导入、骨骼动画、渲染等
	class Mesh
	{
	public:
		Mesh(const std::string& filename); // 从文件构造网格
		~Mesh();

		void OnUpdate(Timestep ts);
		void DumpVertexBuffer();                  // 输出顶点缓冲区信息

		Ref<Shader> GetMeshShader() { return m_MeshShader; }
		Ref<Material> GetMaterial() { return m_BaseMaterial; }
		std::vector<Ref<MaterialInstance>> GetMaterials() { return m_Materials; }
		const std::vector<Ref<Texture2D>>& GetTextures() const { return m_Textures; }
		const std::string& GetFilePath() const { return m_FilePath; }
	private:
		// 骨骼动画相关
		void BoneTransform(float time);
		void ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const glm::mat4& ParentTransform);
		void TraverseNodes(aiNode* node, const glm::mat4& parentTransform = glm::mat4(1.0f), uint32_t level = 0);

		const aiNodeAnim* FindNodeAnim(const aiAnimation* animation, const std::string& nodeName);
		uint32_t FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
		uint32_t FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
		uint32_t FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
		glm::vec3 InterpolateTranslation(float animationTime, const aiNodeAnim* nodeAnim);
		glm::quat InterpolateRotation(float animationTime, const aiNodeAnim* nodeAnim);
		glm::vec3 InterpolateScale(float animationTime, const aiNodeAnim* nodeAnim);
	private:
		std::vector<Submesh> m_Submeshes; // 子网格列表

		std::unique_ptr<Assimp::Importer> m_Importer; // Assimp 导入器

		glm::mat4 m_InverseTransform; // 根节点逆变换

		uint32_t m_BoneCount = 0; // 骨骼数量
		std::vector<BoneInfo> m_BoneInfo; // 骨骼信息

		Ref<VertexArray> m_VertexArray;

		std::vector<Vertex> m_StaticVertices;
		std::vector<AnimatedVertex> m_AnimatedVertices;
		std::vector<Index> m_Indices;   // 索引数据
		std::unordered_map<std::string, uint32_t> m_BoneMapping; // 骨骼名到索引的映射
		std::vector<glm::mat4> m_BoneTransforms; // 骨骼变换矩阵
		const aiScene* m_Scene; // Assimp 场景指针

		// Materials
		Ref<Shader> m_MeshShader;
		Ref<Material> m_BaseMaterial;
		std::vector<Ref<Texture2D>> m_Textures;
		std::vector<Ref<Texture2D>> m_NormalMaps;
		std::vector<Ref<MaterialInstance>> m_Materials;;

		// 动画相关
		bool m_IsAnimated = false;
		float m_AnimationTime = 0.0f;
		float m_WorldTime = 0.0f;
		float m_TimeMultiplier = 1.0f;
		bool m_AnimationPlaying = true;

		std::string m_FilePath;	// 网格文件路径

		friend class Renderer;
		friend class SceneHierarchyPanel;
	};
}
