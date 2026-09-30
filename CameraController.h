#pragma once
#include "KamataEngine.h"

// 前方宣言
class Player;

// 矩形
struct Rect {
	float left = 0.0f;   // 左端
	float right = 1.0f;  // 右端
	float bottom = 0.0f; // 下端
	float top = 1.0f;    // 上端
};

/// <summary>
///	カメラコントローラ
/// </summary>
class CameraController {

public:

	// 追従対象となるプレイヤーから継続的に座標を取得するためのポインタ
	Player* target_ = nullptr;

	// 追従対象とカメラの座標の差 (オフセット)
	KamataEngine::Vector3 targetOffset_ = {0.0f, 0.0f, -15.0f};

	// カメラの目標座標
	KamataEngine::Vector3 targetCoordinate_;

public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// リセット
	/// </summary>
	void Reset();

	// カメラのポインタを取得する関数
	KamataEngine::Camera* GetCamera() { return &camera_; }

	// 外部からポインタをセットするためのsetter
	void SetTarget(Player* target) { target_ = target; }

	// 外部から値をセットできるようにするためのsetter
	void SetMovableArea(Rect area) { movableArea_ = area; }

	// カメラのX座標を固定するためのセッター
	void SetFixedCameraX(bool enabled, float x = 0.0f) {
		isFixedCameraX_ = enabled;
		fixedCameraX_ = x;
	}

private:
	// カメラ
	KamataEngine::Camera camera_;

	// カメラ移動範囲
	Rect movableArea_ = {0.0f, 100.0f, 0.0f, 100.0f};

	// 追従対象が画面外に出ないようにするためのカメラ移動範囲(マージン)
	static inline const Rect margin_ = {-8.0f, 8.0f, -4.5f, 4.5f};

	// 座標補間割合
	static inline const float kInterpolationRate = 0.1f;

	// 速度掛け率
	static inline const float kVelocityBias = 20.0f;

	bool isFixedCameraX_ = false;
	float fixedCameraX_ = 0.0f;
};
