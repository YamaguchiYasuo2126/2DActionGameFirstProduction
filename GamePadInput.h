#pragma once

#include "KamataEngine.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

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

// 突進方向は方向キーを優先し、左スティックは8方向へ量子化する。
inline bool GetAimDirection(int32_t& directionX, int32_t& directionY)
{
	XINPUT_STATE current{};
	XINPUT_STATE previous{};
	if (!GetStates(current, previous))
	{
		return false;
	}
	const WORD buttons = current.Gamepad.wButtons;
	const bool left = (buttons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
	const bool right = (buttons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
	const bool up = (buttons & XINPUT_GAMEPAD_DPAD_UP) != 0;
	const bool down = (buttons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
	if (left || right || up || down)
	{
		directionX = static_cast<int32_t>(right) - static_cast<int32_t>(left);
		directionY = static_cast<int32_t>(up) - static_cast<int32_t>(down);
		return directionX != 0 || directionY != 0;
	}

	const float x = static_cast<float>(current.Gamepad.sThumbLX);
	const float y = static_cast<float>(current.Gamepad.sThumbLY);
	const float deadZone = static_cast<float>(XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
	if (x * x + y * y < deadZone * deadZone)
	{
		return false;
	}
	// 22.5度を境に縦横と斜めを分け、小さな軸ブレで斜めにならないようにする。
	const float threshold = (std::max)(std::abs(x), std::abs(y)) * 0.41421356f;
	directionX = std::abs(x) >= threshold ? (x > 0.0f ? 1 : -1) : 0;
	directionY = std::abs(y) >= threshold ? (y > 0.0f ? 1 : -1) : 0;
	return true;
}

// AとRBのどちらでも方向選択突進を開始できる。RBならXジャンプから親指を動かさずつなげられる。
inline bool IsRushPushed() { return IsButtonPushed(XINPUT_GAMEPAD_A | XINPUT_GAMEPAD_RIGHT_SHOULDER); }
inline bool IsRushTriggered()
{
	XINPUT_STATE current{};
	XINPUT_STATE previous{};
	constexpr WORD kRushButtons = XINPUT_GAMEPAD_A | XINPUT_GAMEPAD_RIGHT_SHOULDER;
	return GetStates(current, previous) &&
		((current.Gamepad.wButtons & ~previous.Gamepad.wButtons) & kRushButtons) != 0;
}
inline bool IsNormalJumpTriggered() { return IsButtonTriggered(XINPUT_GAMEPAD_X); }
inline bool IsGrabTriggered() { return IsButtonTriggered(XINPUT_GAMEPAD_B); }
inline bool IsConfirmTriggered() { return IsButtonTriggered(XINPUT_GAMEPAD_A); }
}
