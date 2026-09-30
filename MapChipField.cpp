#include "MapChipField.h"
#include <map>
#include <fstream>
#include <sstream>
#include <cassert>
#include <cmath>

using namespace KamataEngine;

namespace 
{

// マップチップ種別テーブル
std::map<char, MapChipType> mapChipTypeTable = 
{
    {'B', MapChipType::kBlock},
    {'P', MapChipType::kPlayer},
    {'E', MapChipType::kEnemy},
	{'G', MapChipType::kGimmick},
	{'C', MapChipType::kCheckpoint},
};

}

void MapChipField::ResetMapChipData() 
{
    mapChipData_.data.clear();
	mapChipData_.data.resize(kNumBlockVirtical);
	for (std::vector<MapChipDataUnit>& mapChipDataLine : mapChipData_.data) 
	{
	
		mapChipDataLine.resize(kNumBlockHorizontal);
	}
}

void MapChipField::LoadMapChipCsv(const std::string& filePath)
{
	// マップチップデータをリセット
	ResetMapChipData();

	// ファイルを開く
	std::ifstream file;
	file.open(filePath);
	assert(file.is_open());

	// マップチップCSV
	std::stringstream mapChipCsv;
	// ファイルの内容を文字列ストリームにコピー
	mapChipCsv << file.rdbuf();
	// ファイルを閉じる
	file.close();

	// CSVからマップチップデータを読み込む
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) 
	{
		std::string line;
		getline(mapChipCsv, line);

		// 1行分の文字列をストリームに変換して解析しやすくする
		std::istringstream lineStream(line);

		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j)
		{
		
			std::string word;
			getline(lineStream, word, ',');

			// 空白の場合はスキップ
			if (word.empty())
			{
				continue;
			}
			
			// 先頭文字がいずれかのマップチップ種別に該当するか確認
			if (!mapChipTypeTable.contains(word[kChipType]))
			{
				continue;
			}

			// 先頭文字でマップチップのタイプを判別
			mapChipData_.data[i][j].type = mapChipTypeTable[word[kChipType]];

			// サブIDを含まない場合はスキップ(0番で確定)
			if (word.size() <= kChipSubID)
			{
				continue;
			}

			// マップチップのサブIDを設定
			// G10 のような2桁のサブIDにも対応する。
			mapChipData_.data[i][j].subID = static_cast<uint8_t>(std::stoi(word.substr(kChipSubID)));
		}
	}

}

// 横、縦のインデックス（番号）を指定してその位置のマップチップ種別を取得する関数
MapChipType MapChipField::GetMapChipTypeByIndex(int32_t xIndex, int32_t yIndex)
{
	if (xIndex < 0 || xIndex >= static_cast<int32_t>(kNumBlockHorizontal))
	{
		return MapChipType::kBlank;
	}
	if (yIndex < 0 || yIndex >= static_cast<int32_t>(kNumBlockVirtical))
	{
		return MapChipType::kBlank;
	}

	// 構造体の中の type メンバにアクセスして返すように修正
	return mapChipData_.data[yIndex][xIndex].type;
}

// 横、縦のインデックス（番号）を指定してそのマスのサブIDを返す関数
uint8_t MapChipField::GetMapChipSubIDByIndex(int32_t xIndex, int32_t yIndex)
{
	// 範囲外アクセスの場合はデフォルト値として 0 を返す
	if (xIndex < 0 || xIndex >= static_cast<int32_t>(kNumBlockHorizontal))
	{
		return 0;
	}

	if (yIndex < 0 || yIndex >= static_cast<int32_t>(kNumBlockVirtical))
	{
		return 0;
	}

	// 構造体の中の subID メンバにアクセスして返す
	return mapChipData_.data[yIndex][xIndex].subID;
}

// 指定座標がマップチップの何番の位置にあるのかを計算する関数
IndexSet MapChipField::GetMapChipIndexSetByPosition(const Vector3& position)
{
	IndexSet indexSet = {};
	indexSet.xIndex = static_cast<int32_t>(std::floor((position.x + kBlockWidth / 2.0f) / kBlockWidth));
	const int32_t yIndexFromBottom = static_cast<int32_t>(std::floor((position.y + kBlockHeight / 2.0f) / kBlockHeight));
	indexSet.yIndex = static_cast<int32_t>(kNumBlockVirtical) - 1 - yIndexFromBottom;
	return indexSet;
}

// 横、縦のインデックス（番号）を指定してその位置のマップチップのワールド座標を取得する関数
Vector3 MapChipField::GetMapChipPositionByIndex(int32_t xIndex, int32_t yIndex)
{
	return Vector3(kBlockWidth * static_cast<float>(xIndex), kBlockHeight * static_cast<float>(static_cast<int32_t>(kNumBlockVirtical) - 1 - yIndex), 0);
}

// マップチップ番号を指定して、指定ブロックの全方向の境界の座標を得る関数
MapChipField::Rect MapChipField::GetRectByIndex(int32_t xIndex, int32_t yIndex)
{
	// 指定ブロックの中心座標を取得する
	Vector3 center = GetMapChipPositionByIndex(xIndex, yIndex);

	Rect rect;
	rect.left = center.x - kBlockWidth / 2.0f;
	rect.right = center.x + kBlockWidth / 2.0f;
	rect.bottom = center.y - kBlockHeight / 2.0f;
	rect.top = center.y + kBlockHeight / 2.0f;

	return rect;
}
