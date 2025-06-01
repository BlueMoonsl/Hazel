#include "Hazel.h"

#include "Hazel/ImGui/ImGuiLayer.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

// ImGui 帮助提示工具函数
static void ImGuiShowHelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

// 编辑器主层，负责场景渲染、UI、资源管理等
class EditorLayer : public Hazel::Layer
{
public:
	// 构造函数，初始化清屏色、场景类型和相机
	EditorLayer()
		: m_Scene(Scene::Spheres), m_Camera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 10000.0f))
	{
	}

	virtual ~EditorLayer()
	{
	}

	// 层附加时初始化所有资源（着色器、网格、纹理、帧缓冲等）
	virtual void OnAttach() override
	{
		m_SimplePBRShader.reset(Hazel::Shader::Create("assets/shaders/simplepbr.glsl"));
		m_QuadShader.reset(Hazel::Shader::Create("assets/shaders/quad.glsl"));
		m_HDRShader.reset(Hazel::Shader::Create("assets/shaders/hdr.glsl"));
		m_Mesh.reset(new Hazel::Mesh("assets/meshes/cerberus.fbx"));
		m_SphereMesh.reset(new Hazel::Mesh("assets/models/Sphere.fbx"));

		// 编辑器用棋盘格纹理
		m_CheckerboardTex.reset(Hazel::Texture2D::Create("assets/editor/Checkerboard.tga"));

		// 环境贴图与LUT
		m_EnvironmentCubeMap.reset(Hazel::TextureCube::Create("assets/textures/environments/Arches_E_PineTree_Radiance.tga"));
		//m_EnvironmentCubeMap.reset(Hazel::TextureCube::Create("assets/textures/environments/DebugCubeMap.tga"));
		m_EnvironmentIrradiance.reset(Hazel::TextureCube::Create("assets/textures/environments/Arches_E_PineTree_Irradiance.tga"));
		m_BRDFLUT.reset(Hazel::Texture2D::Create("assets/textures/BRDF_LUT.tga"));

		// hdr帧缓冲与ldr帧缓冲
		m_Framebuffer.reset(Hazel::Framebuffer::Create(1280, 720, Hazel::FramebufferFormat::RGBA16F));
		m_FinalPresentBuffer.reset(Hazel::Framebuffer::Create(1280, 720, Hazel::FramebufferFormat::RGBA8));

		 // 创建全屏四边形顶点和索引缓冲
		float x = -1, y = -1;
		float width = 2, height = 2;
		struct QuadVertex
		{
			glm::vec3 Position;
			glm::vec2 TexCoord;
		};

		QuadVertex* data = new QuadVertex[4];

		data[0].Position = glm::vec3(x, y, 0);
		data[0].TexCoord = glm::vec2(0, 0);

		data[1].Position = glm::vec3(x + width, y, 0);
		data[1].TexCoord = glm::vec2(1, 0);

		data[2].Position = glm::vec3(x + width, y + height, 0);
		data[2].TexCoord = glm::vec2(1, 1);

		data[3].Position = glm::vec3(x, y + height, 0);
		data[3].TexCoord = glm::vec2(0, 1);

		m_VertexBuffer.reset(Hazel::VertexBuffer::Create());
		m_VertexBuffer->SetData(data, 4 * sizeof(QuadVertex));

		uint32_t* indices = new uint32_t[6]{ 0, 1, 2, 2, 3, 0, };
		m_IndexBuffer.reset(Hazel::IndexBuffer::Create());
		m_IndexBuffer->SetData(indices, 6 * sizeof(unsigned int));

		m_Light.Direction = { -0.5f, -0.5f, 1.0f };
		m_Light.Radiance = { 1.0f, 1.0f, 1.0f };
	}

	virtual void OnDetach() override
	{
	}

	// 每帧更新，负责渲染场景到帧缓冲
	virtual void OnUpdate() override
	{
		using namespace Hazel;
		using namespace glm;

		m_Camera.Update();
		auto viewProjection = m_Camera.GetProjectionMatrix() * m_Camera.GetViewMatrix();

		// 1. 渲染环境四边形
		m_Framebuffer->Bind();
		Renderer::Clear();

		Hazel::UniformBufferDeclaration<sizeof(mat4), 1> quadShaderUB;
		quadShaderUB.Push("u_InverseVP", inverse(viewProjection));
		m_QuadShader->UploadUniformBuffer(quadShaderUB);

		m_QuadShader->Bind();
		m_EnvironmentIrradiance->Bind(0);
		m_VertexBuffer->Bind();
		m_IndexBuffer->Bind();
		Renderer::DrawIndexed(m_IndexBuffer->GetCount(), false);

		// 2. 渲染主物体（PBR）
		Hazel::UniformBufferDeclaration<sizeof(mat4) * 2 + sizeof(vec3) * 4 + sizeof(float) * 8, 14> simplePbrShaderUB;
		simplePbrShaderUB.Push("u_ViewProjectionMatrix", viewProjection);
		simplePbrShaderUB.Push("u_ModelMatrix", mat4(1.0f));
		simplePbrShaderUB.Push("u_AlbedoColor", m_AlbedoInput.Color);
		simplePbrShaderUB.Push("u_Metalness", m_MetalnessInput.Value);
		simplePbrShaderUB.Push("u_Roughness", m_RoughnessInput.Value);
		simplePbrShaderUB.Push("lights.Direction", m_Light.Direction);
		simplePbrShaderUB.Push("lights.Radiance", m_Light.Radiance * m_LightMultiplier);
		simplePbrShaderUB.Push("u_CameraPosition", m_Camera.GetPosition());
		simplePbrShaderUB.Push("u_RadiancePrefilter", m_RadiancePrefilter ? 1.0f : 0.0f);
		simplePbrShaderUB.Push("u_AlbedoTexToggle", m_AlbedoInput.UseTexture ? 1.0f : 0.0f);
		simplePbrShaderUB.Push("u_NormalTexToggle", m_NormalInput.UseTexture ? 1.0f : 0.0f);
		simplePbrShaderUB.Push("u_MetalnessTexToggle", m_MetalnessInput.UseTexture ? 1.0f : 0.0f);
		simplePbrShaderUB.Push("u_RoughnessTexToggle", m_RoughnessInput.UseTexture ? 1.0f : 0.0f);
		simplePbrShaderUB.Push("u_EnvMapRotation", m_EnvMapRotation);
		m_SimplePBRShader->UploadUniformBuffer(simplePbrShaderUB);

		m_EnvironmentCubeMap->Bind(10);
		m_EnvironmentIrradiance->Bind(11);
		m_BRDFLUT->Bind(15);

		m_SimplePBRShader->Bind();
		if (m_AlbedoInput.TextureMap)
			m_AlbedoInput.TextureMap->Bind(1);
		if (m_NormalInput.TextureMap)
			m_NormalInput.TextureMap->Bind(2);
		if (m_MetalnessInput.TextureMap)
			m_MetalnessInput.TextureMap->Bind(3);
		if (m_RoughnessInput.TextureMap)
			m_RoughnessInput.TextureMap->Bind(4);

		// 场景切换：球体阵列或模型
		if (m_Scene == Scene::Spheres)
		{
			// 渲染金属球
			float roughness = 0.0f;
			float x = -88.0f;
			for (int i = 0; i < 8; i++)
			{
				m_SimplePBRShader->SetMat4("u_ModelMatrix", translate(mat4(1.0f), vec3(x, 0.0f, 0.0f)));
				m_SimplePBRShader->SetFloat("u_Roughness", roughness);
				m_SimplePBRShader->SetFloat("u_Metalness", 1.0f);
				m_SphereMesh->Render();

				roughness += 0.15f;
				x += 22.0f;
			}

			// 渲染非金属球
			roughness = 0.0f;
			x = -88.0f;
			for (int i = 0; i < 8; i++)
			{
				m_SimplePBRShader->SetMat4("u_ModelMatrix", translate(mat4(1.0f), vec3(x, 22.0f, 0.0f)));
				m_SimplePBRShader->SetFloat("u_Roughness", roughness);
				m_SimplePBRShader->SetFloat("u_Metalness", 0.0f);
				m_SphereMesh->Render();

				roughness += 0.15f;
				x += 22.0f;
			}

		}
		else if (m_Scene == Scene::Model)
		{
			m_Mesh->Render();
		}

		m_Framebuffer->Unbind();

		// 3. HDR后处理与最终输出
		m_FinalPresentBuffer->Bind();
		m_HDRShader->Bind();
		m_HDRShader->SetFloat("u_Exposure", m_Exposure);
		m_Framebuffer->BindTexture();
		m_VertexBuffer->Bind();
		m_IndexBuffer->Bind();
		Renderer::DrawIndexed(m_IndexBuffer->GetCount(), false);
		m_FinalPresentBuffer->Unbind();
	}

	enum class PropertyFlag
	{
		None = 0, ColorProperty = 1
	};

	void Property(const std::string& name, bool& value)
	{
		ImGui::Text(name.c_str());
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		std::string id = "##" + name;
		ImGui::Checkbox(id.c_str(), &value);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
	}

	void Property(const std::string& name, float& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None)
	{
		ImGui::Text(name.c_str());
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		std::string id = "##" + name;
		ImGui::SliderFloat(id.c_str(), &value, min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
	}

	void Property(const std::string& name, glm::vec3& value, PropertyFlag flags)
	{
		Property(name, value, -1.0f, 1.0f, flags);
	}

	void Property(const std::string& name, glm::vec3& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None)
	{
		ImGui::Text(name.c_str());
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		std::string id = "##" + name;
		if ((int)flags & (int)PropertyFlag::ColorProperty)
			ImGui::ColorEdit3(id.c_str(), glm::value_ptr(value), ImGuiColorEditFlags_NoInputs);
		else
			ImGui::SliderFloat3(id.c_str(), glm::value_ptr(value), min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
	}

	void Property(const std::string& name, glm::vec4& value, PropertyFlag flags)
	{
		Property(name, value, -1.0f, 1.0f, flags);
	}

	void Property(const std::string& name, glm::vec4& value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None)
	{
		ImGui::Text(name.c_str());
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		std::string id = "##" + name;
		if ((int)flags & (int)PropertyFlag::ColorProperty)
			ImGui::ColorEdit4(id.c_str(), glm::value_ptr(value), ImGuiColorEditFlags_NoInputs);
		else
			ImGui::SliderFloat4(id.c_str(), glm::value_ptr(value), min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
	}

	// ImGui 渲染，每帧绘制编辑器UI
	virtual void OnImGuiRender() override
	{
		static bool p_open = true;

		static bool opt_fullscreen_persistant = true;
		static ImGuiDockNodeFlags opt_flags = ImGuiDockNodeFlags_None;
		bool opt_fullscreen = opt_fullscreen_persistant;

		// 设置DockSpace窗口属性
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->Pos);
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		// 透传DockSpace背景
		if (opt_flags & ImGuiDockNodeFlags_PassthruDockspace)
			window_flags |= ImGuiWindowFlags_NoBackground;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", &p_open, window_flags);
		ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// DockSpace区域
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), opt_flags);
		}

		// 编辑器设置面板		
		ImGui::Begin("Model");
		ImGui::RadioButton("Spheres", (int*)&m_Scene, (int)Scene::Spheres);
		ImGui::SameLine();
		ImGui::RadioButton("Model", (int*)&m_Scene, (int)Scene::Model);

		ImGui::Begin("Environment");

		ImGui::Columns(2);
			ImGui::AlignTextToFramePadding();

		Property("Light Direction", m_Light.Direction);
		Property("Light Radiance", m_Light.Radiance, PropertyFlag::ColorProperty);
		Property("Light Multiplier", m_LightMultiplier, 0.0f, 5.0f);
		Property("Exposure", m_Exposure, 0.0f, 5.0f);

		Property("Radiance Prefiltering", m_RadiancePrefilter);
		Property("Env Map Rotation", m_EnvMapRotation, -360.0f, 360.0f);

		ImGui::Columns(1);

		ImGui::End();

		ImGui::Separator();
		{
			// 网格资源选择与加载
			ImGui::Text("Mesh");
			std::string fullpath = m_Mesh ? m_Mesh->GetFilePath() : "None";
			size_t found = fullpath.find_last_of("/\\");
			std::string path = found != std::string::npos ? fullpath.substr(found + 1) : fullpath;
			ImGui::Text(path.c_str()); ImGui::SameLine();
			if (ImGui::Button("...##Mesh"))
			{
				std::string filename = Hazel::Application::Get().OpenFile("");
				if (filename != "")
					m_Mesh.reset(new Hazel::Mesh(filename));
			}
		}
		ImGui::Separator();

		// 材质贴图参数面板
		{
			// Albedo
			if (ImGui::CollapsingHeader("Albedo", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
				ImGui::Image(m_AlbedoInput.TextureMap ? (void*)m_AlbedoInput.TextureMap->GetRendererID() : (void*)m_CheckerboardTex->GetRendererID(), ImVec2(64, 64));
				ImGui::PopStyleVar();
				if (ImGui::IsItemHovered())
				{
					if (m_AlbedoInput.TextureMap)
					{
						ImGui::BeginTooltip();
						ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
						ImGui::TextUnformatted(m_AlbedoInput.TextureMap->GetPath().c_str());
						ImGui::PopTextWrapPos();
						ImGui::Image((void*)m_AlbedoInput.TextureMap->GetRendererID(), ImVec2(384, 384));
						ImGui::EndTooltip();
					}
					if (ImGui::IsItemClicked())
					{
						std::string filename = Hazel::Application::Get().OpenFile("");
						if (filename != "")
							m_AlbedoInput.TextureMap.reset(Hazel::Texture2D::Create(filename, m_AlbedoInput.SRGB));
					}
				}
				ImGui::SameLine();
				ImGui::BeginGroup();
				ImGui::Checkbox("Use##AlbedoMap", &m_AlbedoInput.UseTexture);
				if (ImGui::Checkbox("sRGB##AlbedoMap", &m_AlbedoInput.SRGB))
				{
					if (m_AlbedoInput.TextureMap)
						m_AlbedoInput.TextureMap.reset(Hazel::Texture2D::Create(m_AlbedoInput.TextureMap->GetPath(), m_AlbedoInput.SRGB));
				}
				ImGui::EndGroup();
				ImGui::SameLine();
				ImGui::ColorEdit3("Color##Albedo", glm::value_ptr(m_AlbedoInput.Color), ImGuiColorEditFlags_NoInputs);
			}
		}
		{
			// Normals
			if (ImGui::CollapsingHeader("Normals", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
				ImGui::Image(m_NormalInput.TextureMap ? (void*)m_NormalInput.TextureMap->GetRendererID() : (void*)m_CheckerboardTex->GetRendererID(), ImVec2(64, 64));
				ImGui::PopStyleVar();
				if (ImGui::IsItemHovered())
				{
					if (m_NormalInput.TextureMap)
					{
						ImGui::BeginTooltip();
						ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
						ImGui::TextUnformatted(m_NormalInput.TextureMap->GetPath().c_str());
						ImGui::PopTextWrapPos();
						ImGui::Image((void*)m_NormalInput.TextureMap->GetRendererID(), ImVec2(384, 384));
						ImGui::EndTooltip();
					}
					if (ImGui::IsItemClicked())
					{
						std::string filename = Hazel::Application::Get().OpenFile("");
						if (filename != "")
							m_NormalInput.TextureMap.reset(Hazel::Texture2D::Create(filename));
					}
				}
				ImGui::SameLine();
				ImGui::Checkbox("Use##NormalMap", &m_NormalInput.UseTexture);
			}
		}
		{
			// Metalness
			if (ImGui::CollapsingHeader("Metalness", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
				ImGui::Image(m_MetalnessInput.TextureMap ? (void*)m_MetalnessInput.TextureMap->GetRendererID() : (void*)m_CheckerboardTex->GetRendererID(), ImVec2(64, 64));
				ImGui::PopStyleVar();
				if (ImGui::IsItemHovered())
				{
					if (m_MetalnessInput.TextureMap)
					{
						ImGui::BeginTooltip();
						ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
						ImGui::TextUnformatted(m_MetalnessInput.TextureMap->GetPath().c_str());
						ImGui::PopTextWrapPos();
						ImGui::Image((void*)m_MetalnessInput.TextureMap->GetRendererID(), ImVec2(384, 384));
						ImGui::EndTooltip();
					}
					if (ImGui::IsItemClicked())
					{
						std::string filename = Hazel::Application::Get().OpenFile("");
						if (filename != "")
							m_MetalnessInput.TextureMap.reset(Hazel::Texture2D::Create(filename));
					}
				}
				ImGui::SameLine();
				ImGui::Checkbox("Use##MetalnessMap", &m_MetalnessInput.UseTexture);
				ImGui::SameLine();
				ImGui::SliderFloat("Value##MetalnessInput", &m_MetalnessInput.Value, 0.0f, 1.0f);
			}
		}
		{
			// Roughness
			if (ImGui::CollapsingHeader("Roughness", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
				ImGui::Image(m_RoughnessInput.TextureMap ? (void*)m_RoughnessInput.TextureMap->GetRendererID() : (void*)m_CheckerboardTex->GetRendererID(), ImVec2(64, 64));
				ImGui::PopStyleVar();
				if (ImGui::IsItemHovered())
				{
					if (m_RoughnessInput.TextureMap)
					{
						ImGui::BeginTooltip();
						ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
						ImGui::TextUnformatted(m_RoughnessInput.TextureMap->GetPath().c_str());
						ImGui::PopTextWrapPos();
						ImGui::Image((void*)m_RoughnessInput.TextureMap->GetRendererID(), ImVec2(384, 384));
						ImGui::EndTooltip();
					}
					if (ImGui::IsItemClicked())
					{
						std::string filename = Hazel::Application::Get().OpenFile("");
						if (filename != "")
							m_RoughnessInput.TextureMap.reset(Hazel::Texture2D::Create(filename));
					}
				}
				ImGui::SameLine();
				ImGui::Checkbox("Use##RoughnessMap", &m_RoughnessInput.UseTexture);
				ImGui::SameLine();
				ImGui::SliderFloat("Value##RoughnessInput", &m_RoughnessInput.Value, 0.0f, 1.0f);
			}
		}

		ImGui::Separator();

		if (ImGui::TreeNode("Shaders"))
		{
			auto& shaders = Hazel::Shader::s_AllShaders;
				for (auto& shader : shaders)
				{
					if (ImGui::TreeNode(shader->GetName().c_str()))
					{
						std::string buttonName = "Reload##" + shader->GetName();
						if (ImGui::Button(buttonName.c_str()))
							shader->Reload();
						ImGui::TreePop();
					}
				}
			ImGui::TreePop();
		}

		ImGui::End();

		// 视口窗口，显示最终渲染结果
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("Viewport");
		auto viewportSize = ImGui::GetContentRegionAvail();
		m_Framebuffer->Resize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
		m_FinalPresentBuffer->Resize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
		m_Camera.SetProjectionMatrix(glm::perspectiveFov(glm::radians(45.0f), viewportSize.x, viewportSize.y, 0.1f, 10000.0f));
		ImGui::Image((void*)m_FinalPresentBuffer->GetColorAttachmentRendererID(), viewportSize, { 0, 1 }, { 1, 0 });
		ImGui::End();
		ImGui::PopStyleVar();

		// 菜单栏，Docking相关操作
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Docking"))
			{
				// Docking相关标志位切换
				if (ImGui::MenuItem("Flag: NoSplit", "", (opt_flags & ImGuiDockNodeFlags_NoSplit) != 0))                 opt_flags ^= ImGuiDockNodeFlags_NoSplit;
				if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (opt_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))  opt_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode;
				if (ImGui::MenuItem("Flag: NoResize", "", (opt_flags & ImGuiDockNodeFlags_NoResize) != 0))                opt_flags ^= ImGuiDockNodeFlags_NoResize;
				if (ImGui::MenuItem("Flag: PassthruDockspace", "", (opt_flags & ImGuiDockNodeFlags_PassthruDockspace) != 0))       opt_flags ^= ImGuiDockNodeFlags_PassthruDockspace;
				if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (opt_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))          opt_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
				ImGui::Separator();
				if (ImGui::MenuItem("Close DockSpace", NULL, false, p_open != NULL))
					p_open = false;
				ImGui::EndMenu();
			}
			ImGuiShowHelpMarker(
				"You can _always_ dock _any_ window into another by holding the SHIFT key while moving a window. Try it now!" "\n"
				"This demo app has nothing to do with it!" "\n\n"
				"This demo app only demonstrate the use of ImGui::DockSpace() which allows you to manually create a docking node _within_ another window. This is useful so you can decorate your main application window (e.g. with a menu bar)." "\n\n"
				"ImGui::DockSpace() comes with one hard constraint: it needs to be submitted _before_ any window which may be docked into it. Therefore, if you use a dock spot as the central point of your application, you'll probably want it to be part of the very first window you are submitting to imgui every frame." "\n\n"
				"(NB: because of this constraint, the implicit \"Debug\" window can not be docked into an explicit DockSpace() node, because that window is submitted as part of the NewFrame() call. An easy workaround is that you can create your own implicit \"Debug##2\" window after calling DockSpace() and leave it in the window stack for anyone to use.)"
			);

			ImGui::EndMenuBar();
		}

		ImGui::End();
	}

	// 事件处理（预留）
	virtual void OnEvent(Hazel::Event& event) override
	{
	}
private:
	// 着色器与资源
	std::unique_ptr<Hazel::Shader> m_Shader;
	std::unique_ptr<Hazel::Shader> m_PBRShader;
	std::unique_ptr<Hazel::Shader> m_SimplePBRShader;
	std::unique_ptr<Hazel::Shader> m_QuadShader;
	std::unique_ptr<Hazel::Shader> m_HDRShader;
	std::unique_ptr<Hazel::Mesh> m_Mesh;
	std::unique_ptr<Hazel::Mesh> m_SphereMesh;
	std::unique_ptr<Hazel::Texture2D> m_BRDFLUT;

	// 材质输入结构体
	struct AlbedoInput
	{
		glm::vec3 Color = { 0.972f, 0.96f, 0.915f }; // 银色，参考UE文档
		std::unique_ptr<Hazel::Texture2D> TextureMap;
		bool SRGB = true;
		bool UseTexture = false;
	};
	AlbedoInput m_AlbedoInput;

	struct NormalInput
	{
		std::unique_ptr<Hazel::Texture2D> TextureMap;
		bool UseTexture = false;
	};
	NormalInput m_NormalInput;

	struct MetalnessInput
	{
		float Value = 1.0f;
		std::unique_ptr<Hazel::Texture2D> TextureMap;
		bool UseTexture = false;
	};
	MetalnessInput m_MetalnessInput;

	struct RoughnessInput
	{
		float Value = 0.5f;
		std::unique_ptr<Hazel::Texture2D> TextureMap;
		bool UseTexture = false;
	};
	RoughnessInput m_RoughnessInput;

	std::unique_ptr<Hazel::Framebuffer> m_Framebuffer, m_FinalPresentBuffer;

	std::unique_ptr<Hazel::VertexBuffer> m_VertexBuffer;
	std::unique_ptr<Hazel::IndexBuffer> m_IndexBuffer;
	std::unique_ptr<Hazel::TextureCube> m_EnvironmentCubeMap, m_EnvironmentIrradiance;

	Hazel::Camera m_Camera;

	// 光照参数
	struct Light
	{
		glm::vec3 Direction;
		glm::vec3 Radiance;
	};
	Light m_Light;
	float m_LightMultiplier = 0.3f;

	// PBR参数
	float m_Exposure = 1.0f;

	bool m_RadiancePrefilter = false;

	float m_EnvMapRotation = 0.0f;

	// 场景类型
	enum class Scene : uint32_t
	{
		Spheres = 0, Model = 1
	};
	Scene m_Scene;

	// 编辑器资源
	std::unique_ptr<Hazel::Texture2D> m_CheckerboardTex;
};

// 应用程序入口层，负责初始化和启动EditorLayer
class Sandbox : public Hazel::Application
{
public:
	Sandbox()
	{
		HZ_TRACE("Hello!");
	}

	virtual void OnInit() override
	{
		PushLayer(new EditorLayer());
	}
};

// Hazel应用程序工厂函数
Hazel::Application* Hazel::CreateApplication()
{
	return new Sandbox();
}