#include "hzpch.h"
#include "Physics.h"
#include "PXPhysicsWrappers.h"
#include "PhysicsLayer.h"

#include "Hazel/Script/ScriptEngine.h"

#include <PhysX/extensions/PxBroadPhaseExt.h>

namespace Hazel {

	static physx::PxScene* s_Scene;
	static std::vector<Entity> s_SimulatedEntities;
	static Entity* s_EntityStorageBuffer;
	static uint32_t s_EntityBufferCount;
	static int s_EntityStorageBufferPosition;
	static float s_SimulationTime = 0.0F;

	static PhysicsSettings s_Settings;

	void Physics::Init()
	{
		PXPhysicsWrappers::Initialize();
		PhysicsLayerManager::AddLayer("Default");
	}

	void Physics::Shutdown()
	{
		PXPhysicsWrappers::Shutdown();
	}

	void Physics::ExpandEntityBuffer(uint32_t amount)
	{
		HZ_CORE_ASSERT(s_Scene);

		if (s_EntityStorageBuffer != nullptr)
		{
			Entity* temp = new Entity[s_EntityBufferCount + amount];
			memcpy(temp, s_EntityStorageBuffer, s_EntityBufferCount * sizeof(Entity));

			for (uint32_t i = 0; i < s_EntityBufferCount; i++)
			{
				Entity& e = s_EntityStorageBuffer[i];
				RigidBodyComponent& rb = e.GetComponent<RigidBodyComponent>();

				if (rb.RuntimeActor)
				{
					physx::PxRigidActor* actor = static_cast<physx::PxRigidActor*>(rb.RuntimeActor);
					actor->userData = &temp[rb.EntityBufferIndex];
				}
			}

			delete[] s_EntityStorageBuffer;
			s_EntityStorageBuffer = temp;
			s_EntityBufferCount += amount;
		}
		else
		{
			s_EntityStorageBuffer = new Entity[amount];
			s_EntityBufferCount = amount;
		}
	}

	void Physics::CreateScene()
	{
		HZ_CORE_ASSERT(s_Scene == nullptr, "Scene already has a Physics Scene!");
		s_Scene = PXPhysicsWrappers::CreateScene();

		if (s_Settings.BroadphaseAlgorithm != BroadphaseType::AutomaticBoxPrune)
		{
			physx::PxBounds3* regionBounds;
			physx::PxBounds3 globalBounds(ToPhysXVector(s_Settings.WorldBoundsMin), ToPhysXVector(s_Settings.WorldBoundsMax));
			uint32_t regionCount = physx::PxBroadPhaseExt::createRegionsFromWorldBounds(regionBounds, globalBounds, s_Settings.WorldBoundsSubdivisions);
			
			for (uint32_t i = 0; i < regionCount; i++)
			{
				physx::PxBroadPhaseRegion region;
				region.bounds = regionBounds[i];
				s_Scene->addBroadPhaseRegion(region);
			}
		}
	}

	void Physics::CreateActor(Entity e)
	{
		HZ_CORE_ASSERT(s_Scene);

		if (!e.HasComponent<RigidBodyComponent>())
		{
			HZ_CORE_WARN("Trying to create PhysX actor from a non-rigidbody actor!");
			return;
		}

		if (!e.HasComponent<PhysicsMaterialComponent>())
		{
			HZ_CORE_WARN("Trying to create PhysX actor without a PhysicsMaterialComponent!");
			return;
		}

		RigidBodyComponent& rigidbody = e.GetComponent<RigidBodyComponent>();

		TransformComponent& transform = e.GetComponent<TransformComponent>();
		physx::PxRigidActor* actor = PXPhysicsWrappers::CreateActor(rigidbody, transform);

		if (rigidbody.BodyType == RigidBodyComponent::Type::Dynamic)
			s_SimulatedEntities.push_back(e);

		Entity* entityStorage = &s_EntityStorageBuffer[s_EntityStorageBufferPosition];
		*entityStorage = e;
		actor->userData = (void*)entityStorage;
		rigidbody.RuntimeActor = actor;
		rigidbody.EntityBufferIndex = s_EntityStorageBufferPosition;
		s_EntityStorageBufferPosition++;

		physx::PxMaterial* material = PXPhysicsWrappers::CreateMaterial(e.GetComponent<PhysicsMaterialComponent>());

		if (e.HasComponent<BoxColliderComponent>())
		{
			BoxColliderComponent& collider = e.GetComponent<BoxColliderComponent>();
			PXPhysicsWrappers::AddBoxCollider(*actor, *material, collider, transform.Scale);
		}

		if (e.HasComponent<SphereColliderComponent>())
		{
			SphereColliderComponent& collider = e.GetComponent<SphereColliderComponent>();
			PXPhysicsWrappers::AddSphereCollider(*actor, *material, collider, transform.Scale);
		}

		if (e.HasComponent<CapsuleColliderComponent>())
		{
			CapsuleColliderComponent& collider = e.GetComponent<CapsuleColliderComponent>();
			PXPhysicsWrappers::AddCapsuleCollider(*actor, *material, collider, transform.Scale);
		}

		if (e.HasComponent<MeshColliderComponent>())
		{
			MeshColliderComponent& collider = e.GetComponent<MeshColliderComponent>();
			PXPhysicsWrappers::AddMeshCollider(*actor, *material, collider, transform.Scale);
		}

		if (!PhysicsLayerManager::IsLayerValid(rigidbody.Layer))
			rigidbody.Layer = 0;

		PXPhysicsWrappers::SetCollisionFilters(*actor, rigidbody.Layer);
		s_Scene->addActor(*actor);
	}

	PhysicsSettings& Physics::GetSettings()
	{
		return s_Settings;
	}

	void Physics::Simulate(Timestep ts)
	{
		s_SimulationTime += ts.GetMilliseconds();

		if (s_SimulationTime < s_Settings.FixedTimestep)
			return;

		s_SimulationTime -= s_Settings.FixedTimestep;

		for (Entity& e : s_SimulatedEntities)
		{
			if (ScriptEngine::IsEntityModuleValid(e))
				ScriptEngine::OnPhysicsUpdateEntity(e, s_Settings.FixedTimestep);
		}

		s_Scene->simulate(s_Settings.FixedTimestep);
		s_Scene->fetchResults(true);

		for (Entity& e : s_SimulatedEntities)
		{
			TransformComponent& transform = e.Transform();
			RigidBodyComponent& rb = e.GetComponent<RigidBodyComponent>();
			physx::PxRigidActor* actor = static_cast<physx::PxRigidActor*>(rb.RuntimeActor);
			physx::PxTransform actorPose = actor->getGlobalPose();
			transform.Translation = FromPhysXVector(actorPose.p);
			transform.Rotation = glm::eulerAngles(FromPhysXQuat(actorPose.q));
		}
	}

	void Physics::DestroyScene()
	{
		HZ_CORE_ASSERT(s_Scene);

		delete[] s_EntityStorageBuffer;
		s_EntityStorageBuffer = nullptr;
		s_EntityStorageBufferPosition = 0;
		s_SimulatedEntities.clear();
		s_Scene->release();
		s_Scene = nullptr;
	}

	void* Physics::GetPhysicsScene()
	{
		return s_Scene;
	}
}