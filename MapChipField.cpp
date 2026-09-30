#include "MapChipField.h"
#include <map>
#include <fstream>
#include <sstream>
#include <cassert>
#include <cmath>
#include <algorithm>

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

// CSVを分割する関数
std::vector<std::string> SplitCsvLine(const std::string& line) 
{
	std::vector<std::string> cells;

	size_t begin = 0;

	while (true) 
	{
		const size_t commaPosition = line.find(',', begin);

		if (commaPosition == std::string::npos)
		{
			cells.push_back(line.substr(begin));
			break;
		}

		cells.push_back(line.substr(begin, commaPosition - begin));

		begin = commaPosition + 1;
	}

	return cells;
}

}

void MapChipField::ResetMapChipData(uint32_t width, uint32_t height) 
{
	numBlockHorizontal_ = width;
	numBlockVertical_ = height;

	// 以前のデータを破棄して指定サイズの空白マップを作る
	mapChipData_.data.assign(numBlockVertical_, std::vector<MapChipDataUnit>(numBlockHorizontal_, MapChipDataUnit{}));
}

void MapChipField::LoadMapChipCsv(const std::string& filePath)
{
	

	// ファイルを開く
	std::ifstream file(filePath);
	assert(file.is_open() && "マップチップCSVを開けませんでした");

	// 最初に全行を文字列として読み込む
	std::vector<std::vector<std::string>> csvRows;

	std::string line;
	while (std::getline(file, line)) 
	{
		// CRLFのCRが残る環境への対策
		if (!line.empty() && line.back() == '\r')
		{
			line.pop_back();
		}

		csvRows.push_back(SplitCsvLine(line));
	}

	// ファイルを閉じる
	file.close();
	
	assert(!csvRows.empty() && "マップチップCSVが空です");

	// 行数を高さとする
	const uint32_t height = static_cast<uint32_t>(csvRows.size());

	// 最も列数が多い行を幅とする
	uint32_t width = 0;

	for (const std::vector<std::string>& row : csvRows)
	{
		width = (std::max)(width, static_cast<uint32_t>(row.size()));
	}

	assert(width > 0 && "マップチップCSVに列がありません");
	
	// マップチップデータをリセット
	ResetMapChipData(width, height);

	// 読み込んだ文字列をマップチップに変換する
	for (uint32_t y = 0; y < height; ++y) 
	{
		const std::vector<std::string>& row = csvRows[y];

		for (uint32_t x = 0; x < row.size(); ++x) 
		{
			const std::string& word = row[x];

			if (word.empty())
			{
				continue;
			}

			const auto typeIterator = mapChipTypeTable.find(word[kChipType]);

			if (typeIterator == mapChipTypeTable.end())
			{
				continue;
			}

			MapChipDataUnit& mapChip = mapChipData_.data[y][x];

			mapChip.type = typeIterator->second;

			// B、Eなど、サブIDがない場合は0のまま
			if (word.size() <= kChipSubID)
			{
				continue;
			}

			const int32_t subID = std::stoi(word.substr(kChipSubID));

			assert(subID >= 0 && subID <= UINT8_MAX && "マップチップのサブIDが範囲外です");

			mapChip.subID = static_cast<uint8_t>(subID);
		}
	}

}

// 横、縦のインデックス（番号）を指定してその位置のマップチップ種別を取得する関数
MapChipType MapChipField::GetMapChipTypeByIndex(int32_t xIndex, int32_t yIndex) const
{
	if (!IsInBounds(xIndex, yIndex))
	{
		return MapChipType::kBlank;
	}

	// 構造体の中の type メンバにアクセスして返すように修正
	return mapChipData_.data[yIndex][xIndex].type;
}

// 横、縦のインデックス（番号）を指定してそのマスのサブIDを返す関数
uint8_t MapChipField::GetMapChipSubIDByIndex(int32_t xIndex, int32_t yIndex) const
{
	// 範囲外アクセスの場合はデフォルト値として 0 を返す
	if (!IsInBounds(xIndex, yIndex))
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

bool MapChipField::IsInBounds(int32_t xIndex, int32_t yIndex) const
{
	return xIndex >= 0 && xIndex < static_cast<int32_t>(numBlockHorizontal_) && 
		yIndex >= 0 && yIndex < static_cast<int32_t>(numBlockVertical_);
}
