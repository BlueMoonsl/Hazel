using System;
using Hazel;

namespace FPSExample
{
	public class FPSPlayer : Entity
	{
		public float WalkingSpeed = 10.0F;
		public float RunSpeed = 20.0F;
		public float JumpForce = 50.0F;

		[NonSerialized]
		public float MouseSensitivity = 10.0F;

		private bool m_Colliding = false;
		private float m_CurrentSpeed;

		private RigidBodyComponent m_RigidBody;
		private TransformComponent m_Transform;
		private TransformComponent m_CameraTransform;

		private Entity m_CameraEntity;

		private Vector2 m_LastMousePosition;

		private float m_CurrentYMovement = 0.0F;

		void OnCreate()
		{
			m_Transform = GetComponent<TransformComponent>();
			m_RigidBody = GetComponent<RigidBodyComponent>();

			m_CurrentSpeed = WalkingSpeed;

			AddCollisionBeginCallback((n) => { m_Colliding = true; });
			AddCollisionEndCallback((n) => { m_Colliding = false; });

			m_CameraEntity = FindEntityByTag("Camera");
			m_CameraTransform = m_CameraEntity.GetComponent<TransformComponent>();

			m_LastMousePosition = Input.GetMousePosition();

			Input.SetCursorMode(CursorMode.Locked);
		}

		void OnUpdate(float ts)
		{
			if (Input.IsKeyPressed(KeyCode.Escape) && Input.GetCursorMode() == CursorMode.Locked)
				Input.SetCursorMode(CursorMode.Normal);

			if (Input.IsMouseButtonPressed(MouseButton.Left) && Input.GetCursorMode() == CursorMode.Normal)
				Input.SetCursorMode(CursorMode.Locked);
	
			m_CurrentSpeed = Input.IsKeyPressed(KeyCode.LeftControl) ? RunSpeed : WalkingSpeed;

			RaycastHit hitInfo;
			if (Input.IsKeyPressed(KeyCode.H) && Physics.Raycast(m_CameraTransform.Position + (m_CameraTransform.Transform.Forward * 5.0F), m_CameraTransform.Transform.Forward, 20.0F, out hitInfo))
			{
				FindEntityByID(hitInfo.EntityID).GetComponent<MeshComponent>().Mesh.GetMaterial(0).Set("u_AlbedoColor", new Vector3(1.0f, 0.0f, 0.0f));
			}

			if (Input.IsKeyPressed(KeyCode.L))
			{
				// NOTE: The NonAlloc version of Overlap functions should be used when possible since it doesn't allocate a new array
				//			whenever you call it. The normal versions allocates a brand new array every time.

				int numColliders = Physics.OverlapBoxNonAlloc(m_Transform.Position, new Vector3(1.0F), colliders);

				Console.WriteLine("Colliders: {0}", numColliders);

				// When using NonAlloc it's not safe to use a for each loop since some of the colliders may not exist
				for (int i = 0; i < numColliders; i++)
				{
					Console.WriteLine(colliders[i]);
				}
			}

			UpdateRotation(ts);
			UpdateCameraTransform();
		}

		Collider[] colliders = new Collider[10];

		void OnPhysicsUpdate(float fixedTimeStep)
		{
			UpdateMovement();
		}

		private void UpdateRotation(float ts)
		{
			// TODO: Mouse position should be relative to the viewport
			Vector2 currentMousePosition = Input.GetMousePosition();
			Vector2 delta = m_LastMousePosition - currentMousePosition;
			m_CurrentYMovement = delta.X * MouseSensitivity * ts;
			float xRotation = delta.Y * MouseSensitivity * ts;

			if (delta.Y != 0.0F || delta.X != 0.0F)
			{
				m_CameraTransform.Rotation += new Vector3(xRotation, m_CurrentYMovement, 0.0F);
			}

			m_CameraTransform.Rotation = new Vector3(Mathf.Clamp(m_CameraTransform.Rotation.X, -80.0F, 80.0F), m_CameraTransform.Rotation.YZ);

			m_LastMousePosition = currentMousePosition;
		}

		private void UpdateMovement()
		{
			m_RigidBody.Rotate(new Vector3(0.0F, m_CurrentYMovement, 0.0F));

			if (Input.IsKeyPressed(KeyCode.W))
				m_RigidBody.AddForce(m_CameraTransform.Transform.Forward * m_CurrentSpeed);
			else if (Input.IsKeyPressed(KeyCode.S))
				m_RigidBody.AddForce(m_CameraTransform.Transform.Forward * -m_CurrentSpeed);

			if (Input.IsKeyPressed(KeyCode.A))
				m_RigidBody.AddForce(m_CameraTransform.Transform.Right * -m_CurrentSpeed);
			else if (Input.IsKeyPressed(KeyCode.D))
				m_RigidBody.AddForce(m_CameraTransform.Transform.Right * m_CurrentSpeed);

			if (Input.IsKeyPressed(KeyCode.Space) && m_Colliding)
				m_RigidBody.AddForce(Vector3.Up * JumpForce);
		}

		private void UpdateCameraTransform()
		{
			Vector3 position = m_Transform.Position;
			position.Y += 1.5F;
			m_CameraTransform.Position = position;
		}
	}
}
