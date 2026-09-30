#pragma once
#include "KamataEngine.h"

enum class MapChipType 
{
	kBlank,  // 空白
	kBlock,  // ブロック
	kPlayer, // プレイヤー
	kEnemy,  // 敵
	kGimmick, // ステージギミック
	kCheckpoint, // 一方向チェックポイント足場
};

struct MapChipDataUnit
{
	MapChipType type = MapChipType::kBlank; // マップチップの種別
	uint8_t subID = 0;	  // 種類ごとのサブID
};

// マップチップデータ構造体
struct MapChipData
{
	std::vector<std::vector<MapChipDataUnit>> data;
};

struct IndexSet
{
	int32_t xIndex;
	int32_t yIndex;
};

class MapChipField 
{

public:

	// 範囲矩形
	struct Rect
	{
		float left;   // 左端
		float right;  // 右端
		float bottom; // 下端
		float top;    // 上端
	};

	// 1ブロックのサイズ
	static inline const float kBlockWidth = 1.0f;
	static inline const float kBlockHeight = 1.0f;

	// マップチップCSVの文字番号
	enum MapChipCharIndex
	{
		kChipType = 0, // マップチップタイプ
		kChipSubID = 1 // タイプごとのサブID
	};

public:
	
	void ResetMapChipData(uint32_t width, uint32_t height);

	void LoadMapChipCsv(const std::string& filePath);

	// 横、縦のインデックス（番号）を指定してその位置のマップチップ種別を取得する関数
	MapChipType GetMapChipTypeByIndex(int32_t xIndex, int32_t yIndex) const;
	
	// 横、縦のインデックス（番号）を指定してそのマスのサブIDを返す関数
	uint8_t GetMapChipSubIDByIndex(int32_t xIndex, int32_t yIndex) const;

	// 指定座標がマップチップの何番の位置にあるのかを計算する関数
	IndexSet GetMapChipIndexSetByPosition(const KamataEngine::Vector3& position);

	uint32_t GetNumBlockVirtical() const { return numBlockVertical_; }

	uint32_t GetNumBlockHorizontal() const { return numBlockHorizontal_; }

	// 横、縦のインデックス（番号）を指定してその位置のマップチップのワールド座標を取得する関数
	KamataEngine::Vector3 GetMapChipPositionByIndex(int32_t xIndex, int32_t yIndex) const;

	// マップチップ番号を指定して、指定ブロックの全方向の境界の座標を得る関数
	Rect GetRectByIndex(int32_t xIndex, int32_t yIndex) const;
	
private:
	// 範囲判定関数
	bool IsInBounds(int32_t xIndex, int32_t yIndex) const;

private:
	MapChipData mapChipData_;

	uint32_t numBlockVertical_ = 0;
	uint32_t numBlockHorizontal_ = 0;
};
