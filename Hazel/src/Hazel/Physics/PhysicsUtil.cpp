#include "hzpch.h"
#include "PhysicsUtil.h"

#include <filesystem>

namespace Hazel {

	physx::PxTransform ToPhysXTransform(const TransformComponent& transform)
	{
		physx::PxQuat r = ToPhysXQuat(glm::normalize(glm::quat(transform.Rotation)));
		physx::PxVec3 p = ToPhysXVector(transform.Translation);
		return physx::PxTransform(p, r);
	}

	physx::PxTransform ToPhysXTransform(const glm::mat4& transform)
	{
 		physx::PxQuat r = ToPhysXQuat(glm::normalize(glm::toQuat(transform)));
		physx::PxVec3 p = ToPhysXVector(glm::vec3(transform[3]));
		return physx::PxTransform(p, r);
	}

	physx::PxMat44 ToPhysXMatrix(const glm::mat4& matrix)
	{
		return *(physx::PxMat44*)&matrix;
	}

	physx::PxVec3 ToPhysXVector(const glm::vec3& vector)
	{
		return *(physx::PxVec3*)&vector;
	}

	physx::PxVec4 ToPhysXVector(const glm::vec4& vector)
	{
		return *(physx::PxVec4*)&vector;
	}

	physx::PxQuat ToPhysXQuat(const glm::quat& quat)
	{
		return *(physx::PxQuat*)&quat;
	}

	glm::mat4 FromPhysXTransform(const physx::PxTransform& transform)
	{
		glm::quat rotation = FromPhysXQuat(transform.q);
		glm::vec3 position = FromPhysXVector(transform.p);
		return glm::translate(glm::mat4(1.0F), position) * glm::toMat4(rotation);
	}

	glm::mat4 FromPhysXMatrix(const physx::PxMat44& matrix)
	{
		return *(glm::mat4*)&matrix;
	}

	glm::vec3 FromPhysXVector(const physx::PxVec3& vector)
	{
		return *(glm::vec3*)&vector;
	}

	glm::vec4 FromPhysXVector(const physx::PxVec4& vector)
	{
		return *(glm::vec4*)&vector;
	}

	glm::quat FromPhysXQuat(const physx::PxQuat& quat)
	{
		return *(glm::quat*)&quat;
	}

	physx::PxFilterFlags HazelFilterShader(physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0, physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1, physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize)
	{
		if (physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1))
		{
			pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
			return physx::PxFilterFlag::eDEFAULT;
		}

		pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT;

		if ((filterData0.word0 & filterData1.word1) || (filterData1.word0 & filterData0.word1))
		{
			pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_FOUND;
			pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_LOST;
			return physx::PxFilterFlag::eDEFAULT;
		}

		return physx::PxFilterFlag::eSUPPRESS;
	}

	void PhysicsMeshSerializer::DeleteIfSerialized(const std::string& filepath)
	{
		std::filesystem::path p = filepath;
		std::filesystem::path path = p.parent_path() / (p.filename().string() + ".pxm");

		size_t lastDot = path.filename().string().find_first_of(".");
		lastDot = lastDot == std::string::npos ? path.filename().string().length() - 1 : lastDot;
		std::string dirName = p.filename().string().substr(0, lastDot);

		if (IsSerialized(filepath))
			std::filesystem::remove(p.parent_path() / dirName);
	}

	void PhysicsMeshSerializer::SerializeMesh(const std::string& filepath, const physx::PxDefaultMemoryOutputStream& data, const std::string& submeshName)
	{
		std::filesystem::path p = filepath;
		std::filesystem::path path = p.parent_path() / (p.filename().string() + ".pxm");

		size_t lastDot = path.filename().string().find_first_of(".");
		lastDot = lastDot == std::string::npos ? path.filename().string().length() - 1 : lastDot;
		std::string dirName = p.filename().string().substr(0, lastDot);

		if (submeshName.length() > 0)
			path = p.parent_path() / dirName / (submeshName + ".pxm");

		std::filesystem::create_directory(p.parent_path() / dirName);
		std::string cachedFilepath = path.string();

		HZ_CORE_INFO("Serializing {0}", submeshName);

		FILE* f = fopen(cachedFilepath.c_str(), "wb");
		if (f)
		{
			HZ_CORE_INFO("File Created");
			fwrite(data.getData(), sizeof(physx::PxU8), data.getSize() / sizeof(physx::PxU8), f);
			fclose(f);
		}
		else
		{
			HZ_CORE_INFO("File Already Exists");
		}
	}

	bool PhysicsMeshSerializer::IsSerialized(const std::string& filepath)
	{
		std::filesystem::path p = filepath;
		size_t lastDot = p.filename().string().find_first_of(".");
		lastDot = lastDot == std::string::npos ? p.filename().string().length() - 1 : lastDot;
		std::string dirName = p.filename().string().substr(0, lastDot);
		auto path = p.parent_path() / dirName;
		return std::filesystem::is_directory(path);
	}

	static std::vector<physx::PxU8*> s_MeshDataBuffers;

	std::vector<physx::PxDefaultMemoryInputData> PhysicsMeshSerializer::DeserializeMesh(const std::string& filepath)
	{
		std::vector<physx::PxDefaultMemoryInputData> result;

		std::filesystem::path p = filepath;
		size_t lastDot = p.filename().string().find_first_of(".");
		lastDot = lastDot == std::string::npos ? p.filename().string().length() - 1 : lastDot;
		std::string dirName = p.filename().string().substr(0, lastDot);
		auto path = p.parent_path() / dirName;

		for (const auto& file : std::filesystem::directory_iterator(path))
		{
			HZ_CORE_INFO("De-Serializing {0}", file.path().string());

			FILE* f = fopen(file.path().string().c_str(), "rb");
			uint32_t size;

			if (f)
			{
				fseek(f, 0, SEEK_END);
				size = ftell(f);
				fseek(f, 0, SEEK_SET);

				physx::PxU8* buffer = new physx::PxU8[size / sizeof(physx::PxU8)];
				fread(buffer, sizeof(physx::PxU8), size / sizeof(physx::PxU8), f);
				fclose(f);
				s_MeshDataBuffers.push_back(buffer);
				result.push_back(physx::PxDefaultMemoryInputData(buffer, size));
			}
		}

		return result;
	}

	void PhysicsMeshSerializer::CleanupDataBuffers()
	{
		for (auto buffer : s_MeshDataBuffers)
			delete[] buffer;

		s_MeshDataBuffers.clear();
	}

}