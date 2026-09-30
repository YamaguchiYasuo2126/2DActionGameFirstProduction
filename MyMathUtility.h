#pragma once
#include "KamataEngine.h"
#include "AABB.h"

class MyMathUtility {
public:

	/// <summary>
	/// float型の線形補間
	/// </summary>
	/// <param name="a">開始値</param>
	/// <param name="b">目標値</param>
	/// <param name="t">割合 (0.0f ～ 1.0f)</param>
	static float Lerp(float a, float b, float t);

	//==========================================
	// 3次元ベクトル関数
	//==========================================
	
	// 加算
	static KamataEngine::Vector3 Add(const KamataEngine::Vector3& v1, const KamataEngine::Vector3& v2);

	// 減算
	static KamataEngine::Vector3 Subtract(const KamataEngine::Vector3& v1, const KamataEngine::Vector3& v2);

	// 内積
	static float Dot(const KamataEngine::Vector3& v1, const KamataEngine::Vector3& v2);

	// 長さ
	static float Length(const KamataEngine::Vector3& v);

	// ベクトルのスカラー倍
	static KamataEngine::Vector3 Multiply(float scalar, const KamataEngine::Vector3& v1);

	// 正規化
	static KamataEngine::Vector3 Normalize(const KamataEngine::Vector3& v);
	
	// 座標変換
	static KamataEngine::Vector3 Transform(const KamataEngine::Vector3& vector, const KamataEngine::Matrix4x4& matrix);

	// Vector3型の線形補間
	static KamataEngine::Vector3 Lerp(const KamataEngine::Vector3& a, const KamataEngine::Vector3& b, float t);

	//==========================================
	// 4x4行列関数
	//==========================================
	// 行列の加法
	static KamataEngine::Matrix4x4 Add(const KamataEngine::Matrix4x4& m1, const KamataEngine::Matrix4x4& m2);

	// 行列の減法
	static KamataEngine::Matrix4x4 Subtract(const KamataEngine::Matrix4x4& m1, const KamataEngine::Matrix4x4& m2);

	// 行列の積
	static KamataEngine::Matrix4x4 Multiply(const KamataEngine::Matrix4x4& m1, const KamataEngine::Matrix4x4& m2);

	// 単位行列の作成
	static KamataEngine::Matrix4x4 MakeIdentity4x4();

	// 逆行列
	static KamataEngine::Matrix4x4 Inverse(const KamataEngine::Matrix4x4& m);

	// 平行移動行列の作成
	static KamataEngine::Matrix4x4 MakeTranslateMatrix(const KamataEngine::Vector3& translate);

	// 拡大縮小行列の作成
	static KamataEngine::Matrix4x4 MakeScaleMatrix(const KamataEngine::Vector3& scale);

	// X軸回転行列
	static KamataEngine::Matrix4x4 MakeRotateXMatrix(float radian);

	// Y軸回転行列
	static KamataEngine::Matrix4x4 MakeRotateYMatrix(float radian);

	// Z軸回転行列
	static KamataEngine::Matrix4x4 MakeRotateZMatrix(float radian);

	// 3次元アフィン変換行列の作成（Scale -> Rotate -> Translate）
	static KamataEngine::Matrix4x4 MakeAffineMatrix(const KamataEngine::Vector3& scale, const KamataEngine::Vector3& rotate, const KamataEngine::Vector3& translate);

	// AABB同士の衝突判定
	static bool IsCollision(const AABB& aabb1, const AABB& aabb2);

};
