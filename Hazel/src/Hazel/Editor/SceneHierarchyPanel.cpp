#include "hzpch.h"
#include "SceneHierarchyPanel.h"

#include <imgui.h>
#include <imgui/imgui_internal.h>

#include "Hazel/Core/Application.h"
#include "Hazel/Renderer/Mesh.h"
#include "Hazel/Script/ScriptEngine.h"
#include "Hazel/Physics/PhysicsLayer.h"
#include "Hazel/Physics/PXPhysicsWrappers.h"
#include "Hazel/Renderer/MeshFactory.h"

#include <assimp/scene.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Hazel/ImGui/ImGui.h"

// TODO:
// - Eventually change imgui node IDs to be entity/asset GUID

namespace Hazel {

	glm::mat4 Mat4FromAssimpMat4(const aiMatrix4x4& matrix);

	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
		: m_Context(context)
	{
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& scene)
	{
		m_Context = scene;
		m_SelectionContext = {};
		if (m_SelectionContext && false)
		{
			// Try and find same entity in new scene
			auto& entityMap = m_Context->GetEntityMap();
			UUID selectedEntityID = m_SelectionContext.GetUUID();
			if (entityMap.find(selectedEntityID) != entityMap.end())
				m_SelectionContext = entityMap.at(selectedEntityID);
		}
	}

	void SceneHierarchyPanel::SetSelected(Entity entity)
	{
		m_SelectionContext = entity;
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		ImGui::Begin("Scene Hierarchy");
		if (m_Context)
		{
			uint32_t entityCount = 0, meshCount = 0;
			m_Context->m_Registry.each([&](auto entity)
			{
				Entity e(entity, m_Context.Raw());
				if (e.HasComponent<IDComponent>())
					DrawEntityNode(e);
			});

			if (ImGui::BeginPopupContextWindow(0, 1, false))
			{
				if (ImGui::BeginMenu("Create"))
				{
					if (ImGui::MenuItem("Empty Entity"))
					{
						auto newEntity = m_Context->CreateEntity("Empty Entity");
						SetSelected(newEntity);
					}
					if (ImGui::MenuItem("Mesh"))
					{
						auto newEntity = m_Context->CreateEntity("Mesh");
						newEntity.AddComponent<MeshComponent>();
						SetSelected(newEntity);
					}
					ImGui::Separator();
					if (ImGui::MenuItem("Directional Light"))
					{
						auto newEntity = m_Context->CreateEntity("Directional Light");
						newEntity.AddComponent<DirectionalLightComponent>();
						newEntity.GetComponent<TransformComponent>().Rotation = glm::radians(glm::vec3{ 80.0f, 10.0f, 0.0f });
						SetSelected(newEntity);
					}
					if (ImGui::MenuItem("Sky Light"))
					{
						auto newEntity = m_Context->CreateEntity("Sky Light");
						newEntity.AddComponent<SkyLightComponent>();
						SetSelected(newEntity);
					}
					ImGui::EndMenu();
				}
				ImGui::EndPopup();
			}

			ImGui::End();

			ImGui::Begin("Properties");

			if (m_SelectionContext)
				DrawComponents(m_SelectionContext);
		}
		ImGui::End();

#if TODO
		ImGui::Begin("Mesh Debug");
		if (ImGui::CollapsingHeader(mesh->m_FilePath.c_str()))
		{
			if (mesh->m_IsAnimated)
			{
				if (ImGui::CollapsingHeader("Animation"))
				{
					if (ImGui::Button(mesh->m_AnimationPlaying ? "Pause" : "Play"))
						mesh->m_AnimationPlaying = !mesh->m_AnimationPlaying;

					ImGui::SliderFloat("##AnimationTime", &mesh->m_AnimationTime, 0.0f, (float)mesh->m_Scene->mAnimations[0]->mDuration);
					ImGui::DragFloat("Time Scale", &mesh->m_TimeMultiplier, 0.05f, 0.0f, 10.0f);
				}
			}
		}
		ImGui::End();
#endif
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		const char* name = "Unnamed Entity";
		if (entity.HasComponent<TagComponent>())
			name = entity.GetComponent<TagComponent>().Tag.c_str();

		ImGuiTreeNodeFlags flags = (entity == m_SelectionContext ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
		bool opened = ImGui::TreeNodeEx((void*)(uint32_t)entity, flags, name);
		if (ImGui::IsItemClicked())
		{
			m_SelectionContext = entity;
			if (m_SelectionChangedCallback)
				m_SelectionChangedCallback(m_SelectionContext);
		}

		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete"))
				entityDeleted = true;

			ImGui::EndPopup();
		}
		if (opened)
		{
			// TODO: Children
			ImGui::TreePop();
		}

		// Defer deletion until end of node UI
		if (entityDeleted)
		{
			m_Context->DestroyEntity(entity);
			if (entity == m_SelectionContext)
				m_SelectionContext = {};

			m_EntityDeletedCallback(entity);
		}
	}

	void SceneHierarchyPanel::DrawMeshNode(const Ref<Mesh>& mesh, uint32_t& imguiMeshID)
	{
		static char imguiName[128];
		memset(imguiName, 0, 128);
		sprintf(imguiName, "Mesh##%d", imguiMeshID++);

		// Mesh Hierarchy
		if (ImGui::TreeNode(imguiName))
		{
			auto rootNode = mesh->m_Scene->mRootNode;
			MeshNodeHierarchy(mesh, rootNode);
			ImGui::TreePop();
		}
	}

	static std::tuple<glm::vec3, glm::quat, glm::vec3> GetTransformDecomposition(const glm::mat4& transform)
	{
		glm::vec3 scale, translation, skew;
		glm::vec4 perspective;
		glm::quat orientation;
		glm::decompose(transform, scale, orientation, translation, skew, perspective);

		return { translation, orientation, scale };
	}

	void SceneHierarchyPanel::MeshNodeHierarchy(const Ref<Mesh>& mesh, aiNode* node, const glm::mat4& parentTransform, uint32_t level)
	{
		glm::mat4 localTransform = Mat4FromAssimpMat4(node->mTransformation);
		glm::mat4 transform = parentTransform * localTransform;

		if (ImGui::TreeNode(node->mName.C_Str()))
		{
			{
				auto [translation, rotation, scale] = GetTransformDecomposition(transform);
				ImGui::Text("World Transform");
				ImGui::Text("  Translation: %.2f, %.2f, %.2f", translation.x, translation.y, translation.z);
				ImGui::Text("  Scale: %.2f, %.2f, %.2f", scale.x, scale.y, scale.z);
			}
			{
				auto [translation, rotation, scale] = GetTransformDecomposition(localTransform);
				ImGui::Text("Local Transform");
				ImGui::Text("  Translation: %.2f, %.2f, %.2f", translation.x, translation.y, translation.z);
				ImGui::Text("  Scale: %.2f, %.2f, %.2f", scale.x, scale.y, scale.z);
			}

			for (uint32_t i = 0; i < node->mNumChildren; i++)
				MeshNodeHierarchy(mesh, node->mChildren[i], transform, level + 1);

			ImGui::TreePop();
		}

	}

	template<typename T, typename UIFunction>
	static void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction)
	{
		const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
		if (entity.HasComponent<T>())
		{
			// NOTE(Peter):
			//	This fixes an issue where the first "+" button would display the "Remove" buttons for ALL components on an Entity.
			//	This is due to ImGui::TreeNodeEx only pushing the id for it's children if it's actually open
			ImGui::PushID((void*)typeid(T).hash_code());
			auto& component = entity.GetComponent<T>();
			ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImGui::Separator();
			bool open = ImGui::TreeNodeEx("##dummy_id", treeNodeFlags, name.c_str());
			ImGui::PopStyleVar();
			ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
			if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
			{
				ImGui::OpenPopup("ComponentSettings");
			}

			bool removeComponent = false;
			if (ImGui::BeginPopup("ComponentSettings"))
			{
				if (ImGui::MenuItem("Remove component"))
					removeComponent = true;

				ImGui::EndPopup();
			}

			if (open)
			{
				uiFunction(component);
				ImGui::TreePop();
			}

			if (removeComponent)
				entity.RemoveComponent<T>();

			ImGui::PopID();
		}
	}

	static bool DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
	{
		bool modified = false;

		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
		{
			values.x = resetValue;
			modified = true;
		}

		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		modified |= ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
		{
			values.y = resetValue;
			modified = true;
		}

		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		modified |= ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Z", buttonSize))
		{
			values.z = resetValue;
			modified = true;
		}

		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		modified |= ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();

		return modified;
	}

	void SceneHierarchyPanel::DrawComponents(Entity entity)
	{
		ImGui::AlignTextToFramePadding();

		auto id = entity.GetComponent<IDComponent>().ID;

		ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

		if (entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;
			char buffer[256];
			memset(buffer, 0, 256);
			memcpy(buffer, tag.c_str(), tag.length());
			ImGui::PushItemWidth(contentRegionAvailable.x * 0.5f);
			if (ImGui::InputText("##Tag", buffer, 256))
			{
				tag = std::string(buffer);
			}
			ImGui::PopItemWidth();
		}

		// ID
		ImGui::SameLine();
		ImGui::TextDisabled("%llx", id);
		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 textSize = ImGui::CalcTextSize("Add Component");
		ImGui::SameLine(contentRegionAvailable.x - (textSize.x + GImGui->Style.FramePadding.y));
		if (ImGui::Button("Add Component"))
			ImGui::OpenPopup("AddComponentPanel");

		if (ImGui::BeginPopup("AddComponentPanel"))
		{
			if (!m_SelectionContext.HasComponent<CameraComponent>())
			{
				if (ImGui::Button("Camera"))
				{
					m_SelectionContext.AddComponent<CameraComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<MeshComponent>())
			{
				if (ImGui::Button("Mesh"))
				{
					m_SelectionContext.AddComponent<MeshComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<DirectionalLightComponent>())
			{
				if (ImGui::Button("Directional Light"))
				{
					m_SelectionContext.AddComponent<DirectionalLightComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<SkyLightComponent>())
			{
				if (ImGui::Button("Sky Light"))
				{
					m_SelectionContext.AddComponent<SkyLightComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<ScriptComponent>())
			{
				if (ImGui::Button("Script"))
				{
					m_SelectionContext.AddComponent<ScriptComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<SpriteRendererComponent>())
			{
				if (ImGui::Button("Sprite Renderer"))
				{
					m_SelectionContext.AddComponent<SpriteRendererComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<RigidBody2DComponent>())
			{
				if (ImGui::Button("Rigidbody 2D"))
				{
					m_SelectionContext.AddComponent<RigidBody2DComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<BoxCollider2DComponent>())
			{
				if (ImGui::Button("Box Collider 2D"))
				{
					m_SelectionContext.AddComponent<BoxCollider2DComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<CircleCollider2DComponent>())
			{
				if (ImGui::Button("Circle Collider 2D"))
				{
					m_SelectionContext.AddComponent<CircleCollider2DComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<RigidBodyComponent>())
			{
				if (ImGui::Button("Rigidbody"))
				{
					m_SelectionContext.AddComponent<RigidBodyComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<PhysicsMaterialComponent>())
			{
				if (ImGui::Button("Physics Material"))
				{
					m_SelectionContext.AddComponent<PhysicsMaterialComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<BoxColliderComponent>())
			{
				if (ImGui::Button("Box Collider"))
				{
					m_SelectionContext.AddComponent<BoxColliderComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<SphereColliderComponent>())
			{
				if (ImGui::Button("Sphere Collider"))
				{
					m_SelectionContext.AddComponent<SphereColliderComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<SphereColliderComponent>())
			{
				if (ImGui::Button("Capsule Collider"))
				{
					m_SelectionContext.AddComponent<CapsuleColliderComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			if (!m_SelectionContext.HasComponent<MeshColliderComponent>())
			{
				if (ImGui::Button("Mesh Collider"))
				{
					m_SelectionContext.AddComponent<MeshColliderComponent>();
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}

		DrawComponent<TransformComponent>("Transform", entity, [](TransformComponent& component)
		{
			DrawVec3Control("Translation", component.Translation);
			glm::vec3 rotation = glm::degrees(component.Rotation);
			DrawVec3Control("Rotation", rotation);
			component.Rotation = glm::radians(rotation);
			DrawVec3Control("Scale", component.Scale, 1.0f);
		});

		DrawComponent<MeshComponent>("Mesh", entity, [](MeshComponent& mc)
		{
			ImGui::Columns(3);
			ImGui::SetColumnWidth(0, 100);
			ImGui::SetColumnWidth(1, 300);
			ImGui::SetColumnWidth(2, 40);
			ImGui::Text("File Path");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			if (mc.Mesh)
				ImGui::InputText("##meshfilepath", (char*)mc.Mesh->GetFilePath().c_str(), 256, ImGuiInputTextFlags_ReadOnly);
			else
				ImGui::InputText("##meshfilepath", (char*)"Null", 256, ImGuiInputTextFlags_ReadOnly);
			ImGui::PopItemWidth();
			ImGui::NextColumn();
			if (ImGui::Button("...##openmesh"))
			{
				std::string file = Application::Get().OpenFile();
				if (!file.empty())
					mc.Mesh = Ref<Mesh>::Create(file);
			}
			ImGui::Columns(1);
		});

		DrawComponent<CameraComponent>("Camera", entity, [](CameraComponent& cc)
		{
			// Projection Type
			const char* projTypeStrings[] = { "Perspective", "Orthographic" };
			const char* currentProj = projTypeStrings[(int)cc.Camera.GetProjectionType()];
			if (ImGui::BeginCombo("Projection", currentProj))
			{
				for (int type = 0; type < 2; type++)
				{
					bool is_selected = (currentProj == projTypeStrings[type]);
					if (ImGui::Selectable(projTypeStrings[type], is_selected))
					{
						currentProj = projTypeStrings[type];
						cc.Camera.SetProjectionType((SceneCamera::ProjectionType)type);
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			UI::BeginPropertyGrid();
			// Perspective parameters
			if (cc.Camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
			{
				float verticalFOV = cc.Camera.GetPerspectiveVerticalFOV();
				if (UI::Property("Vertical FOV", verticalFOV))
					cc.Camera.SetPerspectiveVerticalFOV(verticalFOV);

				float nearClip = cc.Camera.GetPerspectiveNearClip();
				if (UI::Property("Near Clip", nearClip))
					cc.Camera.SetPerspectiveNearClip(nearClip);
				ImGui::SameLine();
				float farClip = cc.Camera.GetPerspectiveFarClip();
				if (UI::Property("Far Clip", farClip))
					cc.Camera.SetPerspectiveFarClip(farClip);
			}

			// Orthographic parameters
			else if (cc.Camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
			{
				float orthoSize = cc.Camera.GetOrthographicSize();
				if (UI::Property("Size", orthoSize))
					cc.Camera.SetOrthographicSize(orthoSize);

				float nearClip = cc.Camera.GetOrthographicNearClip();
				if (UI::Property("Near Clip", nearClip))
					cc.Camera.SetOrthographicNearClip(nearClip);
				ImGui::SameLine();
				float farClip = cc.Camera.GetOrthographicFarClip();
				if (UI::Property("Far Clip", farClip))
					cc.Camera.SetOrthographicFarClip(farClip);
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<SpriteRendererComponent>("Sprite Renderer", entity, [](SpriteRendererComponent& mc)
		{
		});

		DrawComponent<DirectionalLightComponent>("Directional Light", entity, [](DirectionalLightComponent& dlc)
		{
			UI::BeginPropertyGrid();
			UI::PropertyColor("Radiance", dlc.Radiance);
			UI::Property("Intensity", dlc.Intensity);
			UI::Property("Cast Shadows", dlc.CastShadows);
			UI::Property("Soft Shadows", dlc.SoftShadows);
			UI::Property("Source Size", dlc.LightSize);
			UI::EndPropertyGrid();
		});

		DrawComponent<SkyLightComponent>("Sky Light", entity, [](SkyLightComponent& slc)
		{
			ImGui::Columns(3);
			ImGui::SetColumnWidth(0, 100);
			ImGui::SetColumnWidth(1, 300);
			ImGui::SetColumnWidth(2, 40);
			ImGui::Text("File Path");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			if (!slc.SceneEnvironment.FilePath.empty())
				ImGui::InputText("##envfilepath", (char*)slc.SceneEnvironment.FilePath.c_str(), 256, ImGuiInputTextFlags_ReadOnly);
			else
				ImGui::InputText("##envfilepath", (char*)"Empty", 256, ImGuiInputTextFlags_ReadOnly);
			ImGui::PopItemWidth();
			ImGui::NextColumn();
			if (ImGui::Button("...##openenv"))
			{
				std::string file = Application::Get().OpenFile("*.hdr");
				if (!file.empty())
					slc.SceneEnvironment = Environment::Load(file);
			}
			ImGui::Columns(1);
			
			UI::BeginPropertyGrid();
			UI::Property("Intensity", slc.Intensity, 0.01f, 0.0f, 5.0f);
			UI::EndPropertyGrid();
		});

		DrawComponent<ScriptComponent>("Script", entity, [=](ScriptComponent& sc) mutable
		{
			UI::BeginPropertyGrid();
			std::string oldName = sc.ModuleName;
			if (UI::Property("Module Name", sc.ModuleName, !ScriptEngine::ModuleExists(sc.ModuleName))) // TODO: no live edit
			{
				// Shutdown old script
				if (ScriptEngine::ModuleExists(oldName))
					ScriptEngine::ShutdownScriptEntity(entity, oldName);

				if (ScriptEngine::ModuleExists(sc.ModuleName))
					ScriptEngine::InitScriptEntity(entity);
			}

			// Public Fields
			if (ScriptEngine::ModuleExists(sc.ModuleName))
			{
				EntityInstanceData& entityInstanceData = ScriptEngine::GetEntityInstanceData(entity.GetSceneUUID(), id);
				auto& moduleFieldMap = entityInstanceData.ModuleFieldMap;
				if (moduleFieldMap.find(sc.ModuleName) != moduleFieldMap.end())
				{
					auto& publicFields = moduleFieldMap.at(sc.ModuleName);
					for (auto& [name, field] : publicFields)
					{
						bool isRuntime = m_Context->m_IsPlaying && field.IsRuntimeAvailable();
						switch (field.Type)
						{
						case FieldType::Int:
						{
							int value = isRuntime ? field.GetRuntimeValue<int>() : field.GetStoredValue<int>();
							if (UI::Property(field.Name.c_str(), value))
							{
								if (isRuntime)
									field.SetRuntimeValue(value);
								else
									field.SetStoredValue(value);
							}
							break;
						}
						case FieldType::Float:
						{
							float value = isRuntime ? field.GetRuntimeValue<float>() : field.GetStoredValue<float>();
							if (UI::Property(field.Name.c_str(), value, 0.2f))
							{
								if (isRuntime)
									field.SetRuntimeValue(value);
								else
									field.SetStoredValue(value);
							}
							break;
						}
						case FieldType::Vec2:
						{
							glm::vec2 value = isRuntime ? field.GetRuntimeValue<glm::vec2>() : field.GetStoredValue<glm::vec2>();
							if (UI::Property(field.Name.c_str(), value, 0.2f))
							{
								if (isRuntime)
									field.SetRuntimeValue(value);
								else
									field.SetStoredValue(value);
							}
							break;
						}
						case FieldType::Vec3:
						{
							glm::vec3 value = isRuntime ? field.GetRuntimeValue<glm::vec3>() : field.GetStoredValue<glm::vec3>();
							if (UI::Property(field.Name.c_str(), value, 0.2f))
							{
								if (isRuntime)
									field.SetRuntimeValue(value);
								else
									field.SetStoredValue(value);
							}
							break;
						}
						case FieldType::Vec4:
						{
							glm::vec4 value = isRuntime ? field.GetRuntimeValue<glm::vec4>() : field.GetStoredValue<glm::vec4>();
							if (UI::Property(field.Name.c_str(), value, 0.2f))
							{
								if (isRuntime)
									field.SetRuntimeValue(value);
								else
									field.SetStoredValue(value);
							}
							break;
						}
						}
					}
				}
			}

			UI::EndPropertyGrid();
#if TODO
			if (ImGui::Button("Run Script"))
			{
				ScriptEngine::OnCreateEntity(entity);
			}
#endif
		});

		DrawComponent<RigidBody2DComponent>("Rigidbody 2D", entity, [](RigidBody2DComponent& rb2dc)
		{
			// Rigidbody2D Type
			const char* rb2dTypeStrings[] = { "Static", "Dynamic", "Kinematic" };
			const char* currentType = rb2dTypeStrings[(int)rb2dc.BodyType];
			if (ImGui::BeginCombo("Type", currentType))
			{
				for (int type = 0; type < 3; type++)
				{
					bool is_selected = (currentType == rb2dTypeStrings[type]);
					if (ImGui::Selectable(rb2dTypeStrings[type], is_selected))
					{
						currentType = rb2dTypeStrings[type];
						rb2dc.BodyType = (RigidBody2DComponent::Type)type;
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			if (rb2dc.BodyType == RigidBody2DComponent::Type::Dynamic)
			{
				UI::BeginPropertyGrid();
				UI::Property("Fixed Rotation", rb2dc.FixedRotation);
				UI::EndPropertyGrid();
			}
		});

		DrawComponent<BoxCollider2DComponent>("Box Collider 2D", entity, [](BoxCollider2DComponent& bc2dc)
		{
			UI::BeginPropertyGrid();

			UI::Property("Offset", bc2dc.Offset);
			UI::Property("Size", bc2dc.Size);
			UI::Property("Density", bc2dc.Density);
			UI::Property("Friction", bc2dc.Friction);

			UI::EndPropertyGrid();
		});
	
		DrawComponent<CircleCollider2DComponent>("Circle Collider 2D", entity, [](CircleCollider2DComponent& cc2dc)
		{
			UI::BeginPropertyGrid();

			UI::Property("Offset", cc2dc.Offset);
			UI::Property("Radius", cc2dc.Radius);
			UI::Property("Density", cc2dc.Density);
			UI::Property("Friction", cc2dc.Friction);

			UI::EndPropertyGrid();
		});

		DrawComponent<RigidBodyComponent>("Rigidbody", entity, [](RigidBodyComponent& rbc)
		{
			// Rigidbody Type
			const char* rbTypeStrings[] = { "Static", "Dynamic" };
			const char* currentType = rbTypeStrings[(int)rbc.BodyType];
			if (ImGui::BeginCombo("Type", currentType))
			{
				for (int type = 0; type < 2; type++)
				{
					bool is_selected = (currentType == rbTypeStrings[type]);
					if (ImGui::Selectable(rbTypeStrings[type], is_selected))
					{
						currentType = rbTypeStrings[type];
						rbc.BodyType = (RigidBodyComponent::Type)type;
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			// Layer has been removed, set to Default layer
			if (!PhysicsLayerManager::IsLayerValid(rbc.Layer))
				rbc.Layer = 0;

			uint32_t currentLayer = rbc.Layer;
			const PhysicsLayer& layerInfo = PhysicsLayerManager::GetLayer(currentLayer);
			if (ImGui::BeginCombo("Layer", layerInfo.Name.c_str()))
			{
				for (const auto& layer : PhysicsLayerManager::GetLayers())
				{
					bool is_selected = (currentLayer == layer.LayerID);
					if (ImGui::Selectable(layer.Name.c_str(), is_selected))
					{
						currentLayer = layer.LayerID;
						rbc.Layer = layer.LayerID;
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			if (rbc.BodyType == RigidBodyComponent::Type::Dynamic)
			{
				UI::BeginPropertyGrid();
				UI::Property("Mass", rbc.Mass);
				UI::Property("Is Kinematic", rbc.IsKinematic);
				UI::EndPropertyGrid();

				if (UI::BeginTreeNode("Constraints", false))
				{
					UI::BeginPropertyGrid();
					UI::Property("Position: X", rbc.LockPositionX);
					UI::Property("Position: Y", rbc.LockPositionY);
					UI::Property("Position: Z", rbc.LockPositionZ);
					UI::Property("Rotation: X", rbc.LockRotationX);
					UI::Property("Rotation: Y", rbc.LockRotationY);
					UI::Property("Rotation: Z", rbc.LockRotationZ);
					UI::EndPropertyGrid();

					UI::EndTreeNode();
				}
			}
		});

		DrawComponent<PhysicsMaterialComponent>("Physics Material", entity, [](PhysicsMaterialComponent& pmc)
		{
			UI::BeginPropertyGrid();

			UI::Property("Static Friction", pmc.StaticFriction);
			UI::Property("Dynamic Friction", pmc.DynamicFriction);
			UI::Property("Bounciness", pmc.Bounciness);

			UI::EndPropertyGrid();
		});

		DrawComponent<BoxColliderComponent>("Box Collider", entity, [](BoxColliderComponent& bcc)
		{
			UI::BeginPropertyGrid();

			if (UI::Property("Size", bcc.Size))
			{
				bcc.DebugMesh = MeshFactory::CreateBox(bcc.Size);
			}

			//Property("Offset", bcc.Offset);
			UI::Property("Is Trigger", bcc.IsTrigger);

			UI::EndPropertyGrid();
		});

		DrawComponent<SphereColliderComponent>("Sphere Collider", entity, [](SphereColliderComponent& scc)
		{
			UI::BeginPropertyGrid();

			if (UI::Property("Radius", scc.Radius))
			{
				scc.DebugMesh = MeshFactory::CreateSphere(scc.Radius);
			}

			UI::Property("Is Trigger", scc.IsTrigger);

			UI::EndPropertyGrid();
		});

		DrawComponent<CapsuleColliderComponent>("Capsule Collider", entity, [=](CapsuleColliderComponent& ccc)
		{
			UI::BeginPropertyGrid();

			bool changed = false;

			if (UI::Property("Radius", ccc.Radius))
				changed = true;

			if (UI::Property("Height", ccc.Height))
				changed = true;

			UI::Property("Is Trigger", ccc.IsTrigger);

			if (changed)
			{
				ccc.DebugMesh = MeshFactory::CreateCapsule(ccc.Radius, ccc.Height);
			}

			UI::EndPropertyGrid();
		});

		DrawComponent<MeshColliderComponent>("Mesh Collider", entity, [](MeshColliderComponent& mcc)
		{
			ImGui::Columns(3);
			ImGui::SetColumnWidth(0, 100);
			ImGui::SetColumnWidth(1, 300);
			ImGui::SetColumnWidth(2, 40);
			ImGui::Text("File Path");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			if (mcc.CollisionMesh)
				ImGui::InputText("##meshfilepath", (char*)mcc.CollisionMesh->GetFilePath().c_str(), 256, ImGuiInputTextFlags_ReadOnly);
			else
				ImGui::InputText("##meshfilepath", (char*)"Null", 256, ImGuiInputTextFlags_ReadOnly);
			ImGui::PopItemWidth();
			ImGui::NextColumn();
			if (ImGui::Button("...##openmesh"))
			{
				std::string file = Application::Get().OpenFile();
				if (!file.empty())
				{
					mcc.CollisionMesh = Ref<Mesh>::Create(file);
					if (mcc.IsConvex)
						PXPhysicsWrappers::CreateConvexMesh(mcc, true);
					else
						PXPhysicsWrappers::CreateTriangleMesh(mcc, true);
				}
			}
			ImGui::Columns(1);

			UI::BeginPropertyGrid();
			if (UI::Property("Is Convex", mcc.IsConvex))
			{
				if (mcc.IsConvex)
					PXPhysicsWrappers::CreateConvexMesh(mcc, true);
				else
					PXPhysicsWrappers::CreateTriangleMesh(mcc, true);
			}

			UI::Property("Is Trigger", mcc.IsTrigger);
			UI::EndPropertyGrid();
		});

	}

}
