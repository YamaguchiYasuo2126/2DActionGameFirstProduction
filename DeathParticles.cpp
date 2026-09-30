#define NOMINMAX
#include "DeathParticles.h"
#include "EasingUtility.h"
#include "MapChipField.h"
#include "MyMathUtility.h"
#include <algorithm>
#include <cassert>
#include <numbers>

using namespace KamataEngine;

void DeathParticles::Initialize(Model* model, Camera* camera, const Vector3& position)
{

	// NULLポインタチェック
	assert(model);

	model_ = model;
	camera_ = camera;

	// ワールド変換の初期化
	for (WorldTransform& worldTransform : worldTransforms_)
	{
		worldTransform.Initialize();
		worldTransform.translation_ = position;
	}

	objectColor_.Initialize();
	color_ = {1, 1, 1, 1};
}

void DeathParticles::Update()
{
	// 終了なら何もしない
	if (isFinished_)
	{
		return;
	}

	// カウンターを1フレーム分の秒数進める
	counter_ += 1.0f / 60.0f;

	// 存続時間の上限に達したら
	if (counter_ >= kDuration)
	{
		counter_ = kDuration;
		// 終了扱いにする
		isFinished_ = true;
	}

	for (uint32_t i = 0; i < kNumParticles; ++i)
	{
		// 基本となる速度ベクトル
		Vector3 velocity = {kSpeed, 0.0f, 0.0f};
		// 回転角を計算する
		float angle = kAngleUnit * i;
		// Z軸まわり回転行列
		Matrix4x4 matrixRotation = MyMathUtility::MakeRotateZMatrix(angle);
		// 基本ベクトルを回転させて速度ベクトルを得る
		velocity = MyMathUtility::Transform(velocity, matrixRotation);
		// 移動処理
		worldTransforms_[i].translation_.x += velocity.x;
		worldTransforms_[i].translation_.y += velocity.y;
		worldTransforms_[i].translation_.z += velocity.z;
	}

	// アフィン変換行列の作成
	for (WorldTransform& worldTransform : worldTransforms_)
	{
		worldTransform.matWorld_ = MyMathUtility::MakeAffineMatrix(worldTransform.scale_, worldTransform.rotation_, worldTransform.translation_);

		// 行列を定数バッファに転送
		worldTransform.TransferMatrix();
	}

	// 徐々に薄くする
	color_.w = std::clamp(1.0f - (counter_ / kDuration), 0.0f, 1.0f);
	// 色変更オブジェクトに色の数値を設定する
	objectColor_.SetColor(color_);
}

void DeathParticles::Draw()
{
	// 終了なら何もしない
	if (isFinished_)
	{
		return;
	}

	// モデルの描画
	for (WorldTransform& worldTransform : worldTransforms_) 
	{
		model_->Draw(worldTransform, *camera_, &objectColor_);
	}
}