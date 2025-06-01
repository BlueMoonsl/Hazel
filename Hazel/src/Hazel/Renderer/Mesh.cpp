#include "hzpch.h"
#include "Mesh.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>

#include <glad/glad.h>

namespace Hazel {

	namespace {
		// Assimp 导入标志，控制模型导入时的处理流程
		const unsigned int ImportFlags =
			aiProcess_CalcTangentSpace |      // 计算切线空间
			aiProcess_Triangulate |           // 三角化所有面
			aiProcess_SortByPType |           // 按类型排序
			aiProcess_PreTransformVertices |  // 应用所有变换到顶点
			aiProcess_GenNormals |            // 生成法线
			aiProcess_GenUVCoords |           // 生成UV坐标
			aiProcess_OptimizeMeshes |        // 优化网格
			aiProcess_Debone |                // 移除骨骼影响
			aiProcess_ValidateDataStructure;  // 验证数据结构
	}

	// Assimp 日志流，用于捕获并输出 Assimp 的错误和警告信息
	struct LogStream : public Assimp::LogStream
	{
		// 初始化日志流，只在首次调用时创建
		static void Initialize()
		{
			if (Assimp::DefaultLogger::isNullLogger())
			{
				Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
				Assimp::DefaultLogger::get()->attachStream(new LogStream, Assimp::Logger::Err | Assimp::Logger::Warn);
			}
		}

		// 重写 write 方法，将日志输出到 Hazel 的日志系统
		void write(const char* message) override
		{
			HZ_CORE_ERROR("Assimp error: {0}", message);
		}
	};
	
	// 从文件构造Mesh
	Mesh::Mesh(const std::string& filename)
		: m_FilePath(filename)
	{
		LogStream::Initialize();

		HZ_CORE_INFO("Loading mesh: {0}", filename.c_str());

		Assimp::Importer importer;

		const aiScene* scene = importer.ReadFile(filename, ImportFlags);
		if (!scene || !scene->HasMeshes())
			HZ_CORE_ERROR("Failed to load mesh file: {0}", filename);

		aiMesh* mesh = scene->mMeshes[0];

		HZ_CORE_ASSERT(mesh->HasPositions(), "Meshes require positions.");
		HZ_CORE_ASSERT(mesh->HasNormals(), "Meshes require normals.");

		m_Vertices.reserve(mesh->mNumVertices);

		// 从模型提取顶点数据
		for (size_t i = 0; i < m_Vertices.capacity(); i++)
		{
			Vertex vertex;
			vertex.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
			vertex.Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };

			if (mesh->HasTangentsAndBitangents())
			{
				vertex.Tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
				vertex.Binormal = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
			}

			if (mesh->HasTextureCoords(0))
				vertex.Texcoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
			m_Vertices.push_back(vertex);
		}

		// 创建并上传顶点缓冲区
		m_VertexBuffer.reset(VertexBuffer::Create());
		m_VertexBuffer->SetData(m_Vertices.data(), m_Vertices.size() * sizeof(Vertex));

		// 从文件提取索引数据
		m_Indices.reserve(mesh->mNumFaces);
		for (size_t i = 0; i < m_Indices.capacity(); i++)
		{
			HZ_CORE_ASSERT(mesh->mFaces[i].mNumIndices == 3, "Must have 3 indices.");
			m_Indices.push_back({ mesh->mFaces[i].mIndices[0], mesh->mFaces[i].mIndices[1], mesh->mFaces[i].mIndices[2] });
		}

		m_IndexBuffer.reset(IndexBuffer::Create());
		m_IndexBuffer->SetData(m_Indices.data(), m_Indices.size() * sizeof(Index));
	}

	Mesh::~Mesh()
	{
	}

	// 渲染Mesh，绑定缓冲区并设置顶点属性指针
	void Mesh::Render()
	{
		// TODO: Sort this out
		m_VertexBuffer->Bind();
		m_IndexBuffer->Bind();
		HZ_RENDER_S({
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, Position));

			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, Normal));

			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, Tangent));

			glEnableVertexAttribArray(3);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, Binormal));

			glEnableVertexAttribArray(4);
			glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, Texcoord));
			});
		Renderer::DrawIndexed(m_IndexBuffer->GetCount());
	}

}