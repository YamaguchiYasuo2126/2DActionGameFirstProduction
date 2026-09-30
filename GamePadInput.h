#pragma once

#include "KamataEngine.h"

// Xbox コントローラーの入力を、キーボード入力と併用しやすい形にまとめる。
namespace GamePadInput
{
inline bool GetStates(XINPUT_STATE& current, XINPUT_STATE& previous)
{
	KamataEngine::Input* input = KamataEngine::Input::GetInstance();
	return input->GetJoystickState(0, current) && input->GetJoystickStatePrevious(0, previous);
}

inline bool IsButtonPushed(WORD button)
{
	XINPUT_STATE current{};
	XINPUT_STATE previous{};
	return GetStates(current, previous) && (current.Gamepad.wButtons & button) != 0;
}

inline bool IsButtonTriggered(WORD button)
{
	XINPUT_STATE current{};
	XINPUT_STATE previous{};
	return GetStates(current, previous) &&
		(current.Gamepad.wButtons & button) != 0 &&
		(previous.Gamepad.wButtons & button) == 0;
}

inline bool IsLeftPushed()
{
	XINPUT_STATE current{};
	XINPUT_STATE previous{};
	return GetStates(current, previous) &&
		((current.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0 ||
		 current.Gamepad.sThumbLX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
}

inline bool IsRightPushed()
{
	XINPUT_STATE current{};
	XINPUT_STATE previous{};
	return GetStates(current, previous) &&
		((current.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0 ||
		 current.Gamepad.sThumbLX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
}

inline bool IsLeftTriggered()
{
	XINPUT_STATE current{};
	XINPUT_STATE previous{};
	return GetStates(current, previous) &&
		(((current.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0 &&
		  (previous.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) == 0) ||
		 (current.Gamepad.sThumbLX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
		  previous.Gamepad.sThumbLX >= -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
}

inline bool IsRightTriggered()
{
	XINPUT_STATE current{};
	XINPUT_STATE previous{};
	return GetStates(current, previous) &&
		(((current.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0 &&
		  (previous.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) == 0) ||
		 (current.Gamepad.sThumbLX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
		  previous.Gamepad.sThumbLX <= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
}

inline bool GetLeftStickDirection(KamataEngine::Vector2& direction) {
	XINPUT_STATE current{};
	XINPUT_STATE previous{};

	if (!GetStates(current, previous)) {
		return false;
	}

	const float x = static_cast<float>(current.Gamepad.sThumbLX);
	const float y = static_cast<float>(current.Gamepad.sThumbLY);
	const float deadZone = static_cast<float>(XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

	// スティックが中央付近なら、方向は変更しない
	if (x * x + y * y < deadZone * deadZone) {
		return false;
	}

	direction = {x, y};
	return true;
}

inline bool IsJumpPushed() { return IsButtonPushed(XINPUT_GAMEPAD_A); }
inline bool IsJumpTriggered() { return IsButtonTriggered(XINPUT_GAMEPAD_A); }
inline bool IsAttackTriggered() { return IsButtonTriggered(XINPUT_GAMEPAD_X); }
inline bool IsConfirmTriggered() { return IsButtonTriggered(XINPUT_GAMEPAD_A); }
}
