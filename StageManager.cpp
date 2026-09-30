#include "StageManager.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include <algorithm>

void StageManager::LoadStageDataFile()
{
	// 再読み込み時にも、ステージ数とクリア状態の要素数がずれないようにする。
	stageDatas_.clear();
	clearedStages_.clear();

	// ステージデータファイルのパス
	const std::string filePath = "Resources/stageDatas.csv";

	// ifstreamでステージデータファイルを開く
	std::ifstream file;
	file.open(filePath);
	assert(file.is_open() && "ステージデータファイルが存在しません");

	// ファイルの内容を格納するstringstreamの宣言
	std::stringstream stageCsv;
	// ファイルの内容をstringstreamにコピーする
	stageCsv << file.rdbuf();
	// ファイルを閉じる
	file.close();

	// ステージデータを最終行まで1行ずつ読み込む
	// 1行分の内容を格納するstringの宣言
	std::string line;
	while (std::getline(stageCsv, line)) 
	{
		// 1行分の内容を格納するstringstreamを宣言して、stringから変換
		std::istringstream lineStream(line);

		// ステージデータを格納する構造体
		StageData stageData;
		// カンマ区切りの一つ分を格納するstringの宣言
		std::string word;

		// カンマ区切りで次のデータを取得する
		std::getline(lineStream, word, ',');
		// ステージ名を格納する
		stageData.name = word;

		// カンマ区切りで次のデータを取得する
		std::getline(lineStream, word, ',');
		// 整数に変換して制限時間を格納する
		stageData.timeLimit = std::stoi(word);

		// ステージデータテーブルに格納する
		stageDatas_.push_back(stageData);
	}

	// 読み込んだ全ステージを未クリアで開始する。
	clearedStages_.assign(stageDatas_.size(), false);
}

void StageManager::SetCurrentStageIndexByName(const std::string& name) 
{
	// 全ステージデータを検索
	for (size_t i = 0; i < stageDatas_.size(); ++i)
	{
		// ステージ名が一致したら現在ステージ番号を設定する
		if (stageDatas_[i].name == name)
		{
			currentStageIndex_ = static_cast<int32_t>(i);
			// 目的を達したので関数を抜ける
			return;
		}
	}
	assert(false && "指定されたステージ名は存在しません");
}

void StageManager::MarkStageCleared(int32_t index)
{
	assert(index >= 0 && index < static_cast<int32_t>(clearedStages_.size()) && "ステージ番号が範囲外です");
	clearedStages_[index] = true;
}

bool StageManager::IsStageCleared(int32_t index) const
{
	assert(index >= 0 && index < static_cast<int32_t>(clearedStages_.size()) && "ステージ番号が範囲外です");
	return clearedStages_[index];
}

bool StageManager::AreAllStagesCleared() const
{
	return !clearedStages_.empty() &&
		std::all_of(clearedStages_.begin(), clearedStages_.end(), [](bool isCleared) { return isCleared; });
}

