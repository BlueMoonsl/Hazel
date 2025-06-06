#pragma once

#include <glm/glm.hpp>
#include "Hazel/Core/Timestep.h"

namespace Hazel {

	// Camera 类：用于表示和控制场景中的相机，支持轨道（自由）视角操作
	class Camera
	{
	public:
		Camera() = default;
		Camera(const glm::mat4& projectionMatrix);

		void Focus();				// 使相机聚焦到焦点位置
		void Update(Timestep ts);	// 更新相机状态（如响应输入、更新视图矩阵等）

		// 获取/设置相机与焦点的距离
		inline float GetDistance() const { return m_Distance; }
		inline void SetDistance(float distance) { m_Distance = distance; }

		// 设置投影矩阵
		inline void SetProjectionMatrix(const glm::mat4& projectionMatrix) { m_ProjectionMatrix = projectionMatrix; }
		inline void SetViewportSize(uint32_t width, uint32_t height) { m_ViewportWidth = width; m_ViewportHeight = height; }

		// 获取投影矩阵、视图矩阵
		const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }

		// 获取相机的上、右、前方向向量
		glm::vec3 GetUpDirection();
		glm::vec3 GetRightDirection();
		glm::vec3 GetForwardDirection();
		// 获取相机当前位置
		const glm::vec3& GetPosition() const { return m_Position; }

		float GetExposure() const { return m_Exposure; }
		float& GetExposure() { return m_Exposure; }
	private:
		void MousePan(const glm::vec2& delta);				// 鼠标平移
		void MouseRotate(const glm::vec2& delta);			// 鼠标旋转
		void MouseZoom(float delta);						// 鼠标缩放

		glm::vec3 CalculatePosition();						// 计算相机位置
		glm::quat GetOrientation();							// 获取相机朝向的四元数
	
		std::pair<float, float> PanSpeed() const;
		float RotationSpeed() const;
		float ZoomSpeed() const;
	private:
		glm::mat4 m_ProjectionMatrix, m_ViewMatrix;			// 投影矩阵和视图矩阵
		glm::vec3 m_Position, m_Rotation, m_FocalPoint;		// 相机位置、欧拉角旋转、焦点位置

		bool m_Panning, m_Rotating;							// 是否正在平移/旋转
		glm::vec2 m_InitialMousePosition;					// 鼠标初始位置
		glm::vec3 m_InitialFocalPoint, m_InitialRotation;	// 操作开始时的焦点和旋转

		float m_Distance;									// 相机与焦点的距离
		float m_Pitch, m_Yaw;								// 俯仰角和偏航角（欧拉角）
		
		float m_Exposure = 0.8f;

		uint32_t m_ViewportWidth = 1280, m_ViewportHeight = 720;
	};

}
