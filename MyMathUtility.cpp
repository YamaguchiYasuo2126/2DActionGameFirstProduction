#include "MyMathUtility.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <cassert>

using namespace KamataEngine;

/// <summary>
/// float型の線形補間
/// </summary>
/// <param name="a">開始値</param>
/// <param name="b">目標値</param>
/// <param name="t">割合 (0.0f ～ 1.0f)</param>
float MyMathUtility::Lerp(float a, float b, float t) 
{ 
	return a + (b - a) * t;
}

//==========================================
// 3次元ベクトル関数
//==========================================

// 加算
Vector3 MyMathUtility::Add(const KamataEngine::Vector3& v1, const KamataEngine::Vector3& v2) {
	Vector3 result = {};

	result.x = v1.x + v2.x;
	result.y = v1.y + v2.y;
	result.z = v1.z + v2.z;

	return result;
}


// 減算
Vector3 MyMathUtility::Subtract(const KamataEngine::Vector3& v1, const KamataEngine::Vector3& v2) {
	Vector3 result = {};

	result.x = v1.x - v2.x;
	result.y = v1.y - v2.y;
	result.z = v1.z - v2.z;

	return result;
}

// 内積
float MyMathUtility::Dot(const KamataEngine::Vector3& v1, const KamataEngine::Vector3& v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

// 長さ
float MyMathUtility::Length(const KamataEngine::Vector3& v) {
	return sqrtf(Dot(v, v));
}


// ベクトルのスカラー倍
Vector3 MyMathUtility::Multiply(float scalar, const KamataEngine::Vector3& v1) {
	Vector3 result = {};

	result.x = scalar * v1.x;
	result.y = scalar * v1.y;
	result.z = scalar * v1.z;

	return result;
}


// 正規化
Vector3 MyMathUtility::Normalize(const KamataEngine::Vector3& v) {
	Vector3 result = {};
	float length = Length(v);

	// 0除算防止
	if (length != 0.0f) {
		result.x = v.x / length;
		result.y = v.y / length;
		result.z = v.z / length;
	}

	return result;
}

// 座標変換
Vector3 MyMathUtility::Transform(const KamataEngine::Vector3& vector, const KamataEngine::Matrix4x4& matrix) {
	Vector3 result;
	// 行列の各列とベクトルの成分を掛け合わせる
	result.x = vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0] + vector.z * matrix.m[2][0] + 1.0f * matrix.m[3][0];
	result.y = vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1] + vector.z * matrix.m[2][1] + 1.0f * matrix.m[3][1];
	result.z = vector.x * matrix.m[0][2] + vector.y * matrix.m[1][2] + vector.z * matrix.m[2][2] + 1.0f * matrix.m[3][2];

	// w成分の計算
	float w = vector.x * matrix.m[0][3] + vector.y * matrix.m[1][3] + vector.z * matrix.m[2][3] + 1.0f * matrix.m[3][3];

	// wが0でないことを確認して正規化
	assert(w != 0.0f);
	result.x /= w;
	result.y /= w;
	result.z /= w;

	return result;
}

Vector3 MyMathUtility::Lerp(const KamataEngine::Vector3& a, const KamataEngine::Vector3& b, float t) {
	KamataEngine::Vector3 result;

	// 各成分ごとに線形補間を行う
	result.x = Lerp(a.x, b.x, t);
	result.y = Lerp(a.y, b.y, t);
	result.z = Lerp(a.z, b.z, t);

	return result;
}

//==========================================
// 4x4行列関数
//==========================================
// 行列の加法
Matrix4x4 MyMathUtility::Add(const KamataEngine::Matrix4x4& m1, const KamataEngine::Matrix4x4& m2) {
	Matrix4x4 result = {};
	result.m[0][0] = m1.m[0][0] + m2.m[0][0];
	result.m[0][1] = m1.m[0][1] + m2.m[0][1];
	result.m[0][2] = m1.m[0][2] + m2.m[0][2];
	result.m[0][3] = m1.m[0][3] + m2.m[0][3];
	result.m[1][0] = m1.m[1][0] + m2.m[1][0];
	result.m[1][1] = m1.m[1][1] + m2.m[1][1];
	result.m[1][2] = m1.m[1][2] + m2.m[1][2];
	result.m[1][3] = m1.m[1][3] + m2.m[1][3];
	result.m[2][0] = m1.m[2][0] + m2.m[2][0];
	result.m[2][1] = m1.m[2][1] + m2.m[2][1];
	result.m[2][2] = m1.m[2][2] + m2.m[2][2];
	result.m[2][3] = m1.m[2][3] + m2.m[2][3];
	result.m[3][0] = m1.m[3][0] + m2.m[3][0];
	result.m[3][1] = m1.m[3][1] + m2.m[3][1];
	result.m[3][2] = m1.m[3][2] + m2.m[3][2];
	result.m[3][3] = m1.m[3][3] + m2.m[3][3];
	return result;
}

// 行列の減法
Matrix4x4 MyMathUtility::Subtract(const KamataEngine::Matrix4x4& m1, const KamataEngine::Matrix4x4& m2) {
	Matrix4x4 result = {};
	result.m[0][0] = m1.m[0][0] - m2.m[0][0];
	result.m[0][1] = m1.m[0][1] - m2.m[0][1];
	result.m[0][2] = m1.m[0][2] - m2.m[0][2];
	result.m[0][3] = m1.m[0][3] - m2.m[0][3];
	result.m[1][0] = m1.m[1][0] - m2.m[1][0];
	result.m[1][1] = m1.m[1][1] - m2.m[1][1];
	result.m[1][2] = m1.m[1][2] - m2.m[1][2];
	result.m[1][3] = m1.m[1][3] - m2.m[1][3];
	result.m[2][0] = m1.m[2][0] - m2.m[2][0];
	result.m[2][1] = m1.m[2][1] - m2.m[2][1];
	result.m[2][2] = m1.m[2][2] - m2.m[2][2];
	result.m[2][3] = m1.m[2][3] - m2.m[2][3];
	result.m[3][0] = m1.m[3][0] - m2.m[3][0];
	result.m[3][1] = m1.m[3][1] - m2.m[3][1];
	result.m[3][2] = m1.m[3][2] - m2.m[3][2];
	result.m[3][3] = m1.m[3][3] - m2.m[3][3];
	return result;
}


// 行列の積
Matrix4x4 MyMathUtility::Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result = {};

	// 0行目
	result.m[0][0] = m1.m[0][0] * m2.m[0][0] + m1.m[0][1] * m2.m[1][0] + m1.m[0][2] * m2.m[2][0] + m1.m[0][3] * m2.m[3][0];
	result.m[0][1] = m1.m[0][0] * m2.m[0][1] + m1.m[0][1] * m2.m[1][1] + m1.m[0][2] * m2.m[2][1] + m1.m[0][3] * m2.m[3][1];
	result.m[0][2] = m1.m[0][0] * m2.m[0][2] + m1.m[0][1] * m2.m[1][2] + m1.m[0][2] * m2.m[2][2] + m1.m[0][3] * m2.m[3][2];
	result.m[0][3] = m1.m[0][0] * m2.m[0][3] + m1.m[0][1] * m2.m[1][3] + m1.m[0][2] * m2.m[2][3] + m1.m[0][3] * m2.m[3][3];
	// 1行目
	result.m[1][0] = m1.m[1][0] * m2.m[0][0] + m1.m[1][1] * m2.m[1][0] + m1.m[1][2] * m2.m[2][0] + m1.m[1][3] * m2.m[3][0];
	result.m[1][1] = m1.m[1][0] * m2.m[0][1] + m1.m[1][1] * m2.m[1][1] + m1.m[1][2] * m2.m[2][1] + m1.m[1][3] * m2.m[3][1];
	result.m[1][2] = m1.m[1][0] * m2.m[0][2] + m1.m[1][1] * m2.m[1][2] + m1.m[1][2] * m2.m[2][2] + m1.m[1][3] * m2.m[3][2];
	result.m[1][3] = m1.m[1][0] * m2.m[0][3] + m1.m[1][1] * m2.m[1][3] + m1.m[1][2] * m2.m[2][3] + m1.m[1][3] * m2.m[3][3];
	// 2行目
	result.m[2][0] = m1.m[2][0] * m2.m[0][0] + m1.m[2][1] * m2.m[1][0] + m1.m[2][2] * m2.m[2][0] + m1.m[2][3] * m2.m[3][0];
	result.m[2][1] = m1.m[2][0] * m2.m[0][1] + m1.m[2][1] * m2.m[1][1] + m1.m[2][2] * m2.m[2][1] + m1.m[2][3] * m2.m[3][1];
	result.m[2][2] = m1.m[2][0] * m2.m[0][2] + m1.m[2][1] * m2.m[1][2] + m1.m[2][2] * m2.m[2][2] + m1.m[2][3] * m2.m[3][2];
	result.m[2][3] = m1.m[2][0] * m2.m[0][3] + m1.m[2][1] * m2.m[1][3] + m1.m[2][2] * m2.m[2][3] + m1.m[2][3] * m2.m[3][3];
	// 3行目
	result.m[3][0] = m1.m[3][0] * m2.m[0][0] + m1.m[3][1] * m2.m[1][0] + m1.m[3][2] * m2.m[2][0] + m1.m[3][3] * m2.m[3][0];
	result.m[3][1] = m1.m[3][0] * m2.m[0][1] + m1.m[3][1] * m2.m[1][1] + m1.m[3][2] * m2.m[2][1] + m1.m[3][3] * m2.m[3][1];
	result.m[3][2] = m1.m[3][0] * m2.m[0][2] + m1.m[3][1] * m2.m[1][2] + m1.m[3][2] * m2.m[2][2] + m1.m[3][3] * m2.m[3][2];
	result.m[3][3] = m1.m[3][0] * m2.m[0][3] + m1.m[3][1] * m2.m[1][3] + m1.m[3][2] * m2.m[2][3] + m1.m[3][3] * m2.m[3][3];

	return result;
}

// 単位行列の作成
Matrix4x4 MyMathUtility::MakeIdentity4x4() {
	Matrix4x4 result = {};
	result.m[0][0] = 1.0f;
	result.m[0][1] = 0.0f;
	result.m[0][2] = 0.0f;
	result.m[0][3] = 0.0f;
	result.m[1][0] = 0.0f;
	result.m[1][1] = 1.0f;
	result.m[1][2] = 0.0f;
	result.m[1][3] = 0.0f;
	result.m[2][0] = 0.0f;
	result.m[2][1] = 0.0f;
	result.m[2][2] = 1.0f;
	result.m[2][3] = 0.0f;
	result.m[3][0] = 0.0f;
	result.m[3][1] = 0.0f;
	result.m[3][2] = 0.0f;
	result.m[3][3] = 1.0f;
	return result;
}

// 逆行列(非常に長くなるためfor文で実装)
Matrix4x4 MyMathUtility::Inverse(const Matrix4x4& m) {
	// 単位行列で初期化
	Matrix4x4 result = MakeIdentity4x4();

	// 計算用に元の行列をコピーして使う
	Matrix4x4 tmp = m;

	for (int i = 0; i < 4; ++i) {

		// 現在の列の中で、一番値（絶対値）が大きい行を探す
		// 最大の絶対値を持つ行のインデックス
		int maxValRowIndex = i;
		for (int j = i + 1; j < 4; ++j) {
			// マイナスの場合は-1を掛けて絶対値にする
			float currentAbs = (tmp.m[j][i] < 0.0f) ? -tmp.m[j][i] : tmp.m[j][i];
			float maxAbs = (tmp.m[maxValRowIndex][i] < 0.0f) ? -tmp.m[maxValRowIndex][i] : tmp.m[maxValRowIndex][i];

			if (currentAbs > maxAbs) {
				maxValRowIndex = j;
			}
		}

		// 一番値が大きい行が見つかったら、現在の行と入れ替える
		if (maxValRowIndex != i) {
			for (int j = 0; j < 4; ++j) {
				// 計算用行列の行を入れ替え
				float tempValue = tmp.m[i][j];
				tmp.m[i][j] = tmp.m[maxValRowIndex][j];
				tmp.m[maxValRowIndex][j] = tempValue;

				// 結果用(逆行列)の行も同じように入れ替え
				tempValue = result.m[i][j];
				result.m[i][j] = result.m[maxValRowIndex][j];
				result.m[maxValRowIndex][j] = tempValue;
			}
		}

		// 対象の要素(対角成分)を1にする
		// 対角線上にある数値
		float diagonalValue = tmp.m[i][i];

		if (diagonalValue == 0.0f) {
			// すべて0になってしまった場合は逆行列が存在しない
			return result;
		}

		// その行のすべての成分を対角線上の値で割ることで、対角成分を1にする
		for (int j = 0; j < 4; ++j) {
			tmp.m[i][j] /= diagonalValue;
			result.m[i][j] /= diagonalValue;
		}

		// 現在の列の他の行の数値をすべて0にする
		for (int j = 0; j < 4; ++j) {
			// 自分自身の行以外を処理する
			if (i != j) {
				float multiplier = tmp.m[j][i];

				for (int k = 0; k < 4; ++k) {
					tmp.m[j][k] -= tmp.m[i][k] * multiplier;
					result.m[j][k] -= result.m[i][k] * multiplier;
				}
			}
		}
	}

	return result;
}

// 平行移動行列
Matrix4x4 MyMathUtility::MakeTranslateMatrix(const Vector3& translate) {
	Matrix4x4 result = {};
	result.m[0][0] = 1.0f;
	result.m[1][1] = 1.0f;
	result.m[2][2] = 1.0f;
	result.m[3][3] = 1.0f;

	// 4行目に移動量を代入
	result.m[3][0] = translate.x;
	result.m[3][1] = translate.y;
	result.m[3][2] = translate.z;

	return result;
}

// 拡大縮小行列
Matrix4x4 MyMathUtility::MakeScaleMatrix(const Vector3& scale) {
	Matrix4x4 result = {};
	result.m[0][0] = scale.x;
	result.m[1][1] = scale.y;
	result.m[2][2] = scale.z;
	result.m[3][3] = 1.0f;

	return result;
}

// X軸回転行列
Matrix4x4 MyMathUtility::MakeRotateXMatrix(float radian) {
	// 単位行列で初期化
	Matrix4x4 result = MakeIdentity4x4();
	result.m[1][1] = std::cos(radian);
	result.m[1][2] = std::sin(radian);
	result.m[2][1] = -std::sin(radian);
	result.m[2][2] = std::cos(radian);
	return result;
}

// Y軸回転行列
Matrix4x4 MyMathUtility::MakeRotateYMatrix(float radian) {
	// 単位行列で初期化
	Matrix4x4 result = MakeIdentity4x4();
	result.m[0][0] = std::cos(radian);
	result.m[0][2] = -std::sin(radian);
	result.m[2][0] = std::sin(radian);
	result.m[2][2] = std::cos(radian);
	return result;
}

// Z軸回転行列
Matrix4x4 MyMathUtility::MakeRotateZMatrix(float radian) {
	// 単位行列で初期化
	Matrix4x4 result = MakeIdentity4x4();
	result.m[0][0] = std::cos(radian);
	result.m[0][1] = std::sin(radian);
	result.m[1][0] = -std::sin(radian);
	result.m[1][1] = std::cos(radian);
	return result;
}

// 3次元アフィン変換行列
Matrix4x4 MyMathUtility::MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {
	// 各行列を作成
	Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);
	Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);

	// 回転行列(X, Y, Zを合成)
	Matrix4x4 rotateX = MakeRotateXMatrix(rotate.x);
	Matrix4x4 rotateY = MakeRotateYMatrix(rotate.y);
	Matrix4x4 rotateZ = MakeRotateZMatrix(rotate.z);

	// X * Y * Z の順で合成
	Matrix4x4 rotateXYZ = Multiply(rotateX, Multiply(rotateY, rotateZ));

	// アフィン変換の順序： Scale -> Rotate -> Translate
	Matrix4x4 scaleRotateMatrix = Multiply(scaleMatrix, rotateXYZ);
	Matrix4x4 worldMatrix = Multiply(scaleRotateMatrix, translateMatrix);

	return worldMatrix;
}

// AABB同士の衝突判定
bool MyMathUtility::IsCollision(const AABB& aabb1, const AABB& aabb2)
{
	// X軸、Y軸、Z軸すべてで重なっているかチェックする
	if ((aabb1.min.x <= aabb2.max.x && aabb1.max.x >= aabb2.min.x) &&
		(aabb1.min.y <= aabb2.max.y && aabb1.max.y >= aabb2.min.y) &&
		(aabb1.min.z <= aabb2.max.z && aabb1.max.z >= aabb2.min.z)) 
	{
		return true; // すべての軸で重なっていれば衝突
	}

	return false;
}
