#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "Hazel/Renderer/Buffer.h"

namespace Hazel {

	class Mesh
	{
	public:
		// 顶点属性结构体
		struct Vertex
		{
			glm::vec3 Position;			// 顶点位置（世界坐标）
			glm::vec3 Normal;			// 顶点法线
			glm::vec3 Tangent;			// 切线
			glm::vec3 Binormal;			// 副法线
			glm::vec2 Texcoord;			// 纹理坐标
		};
		static_assert(sizeof(Vertex) == 14 * sizeof(float)); 					// 静态断言，确保Vertex结构体大小为14个float
		static const int NumAttributes = 5;										// 顶点属性数量

		// 三角形索引结构体
		struct Index
		{
			uint32_t V1, V2, V3;		// 三个顶点索引，组成一个三角形
		};
		static_assert(sizeof(Index) == 3 * sizeof(uint32_t)); 					// 静态断言，确保Index结构体大小为3个uint32_t

		Mesh(const std::string& filename);
		~Mesh();

		void Render();

		inline const std::string& GetFilePath() const { return m_FilePath; }	// 获取网格文件路径
	private:
		std::vector<Vertex> m_Vertices;				  // 顶点数据
		std::vector<Index> m_Indices;				  // 索引数据（三角形）

		std::unique_ptr<VertexBuffer> m_VertexBuffer; // 顶点缓冲区对象
		std::unique_ptr<IndexBuffer> m_IndexBuffer;   // 索引缓冲区对象

		std::string m_FilePath;						  // 网格文件路径
	};
}
