#pragma once

namespace Hazel {

	class Input
	{
	public:
		static bool IsKeyPressed(int keycode) { return s_Instance->IsKeyPressed(keycode); }

		inline static bool IsMouseButtonPressed(int button) { return s_Instance->IsMouseButtonPressed(button); }
		inline static float GetMouseX() { return s_Instance->GetMouseX(); }
		inline static float GetMouseY() { return s_Instance->GetMouseY(); }
	protected:
		virtual bool IsKeyPressedImpl(int keycode) = 0;
		virtual bool IsMouseButtonPressedImpl(int button) = 0;
		virtual float GetMouseXImpl() = 0;
		virtual float GetMouseYImpl() = 0;
	private:
		static Input* s_Instance;
	};

}