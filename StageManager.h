#pragma once

#include <cassert>
#include "KamataEngine.h"
#include <vector>
#include <string>

struct StageData {
	std::string name;  // ステージ名(フィールドCSVファイル名)
	int32_t timeLimit; // 制限時間
};

/// <summary>
/// ステージ管理
/// </summary>
class StageManager 
{

public:
	
	/// <summary>
	/// ステージデータファイルの読み込み
	/// </summary>
	void LoadStageDataFile();

	/// <summary>
	/// ステージデータの取得
	/// </summary>
	/// <param name="index">ステージ番号</param>
	/// <returns>ステージデータ</returns>
	const StageData& GetStageData(int32_t index) const
	{
		// indexが0以上、かつstageDatas_の要素数より小さいことを確認する
		assert(index >= 0 && index < stageDatas_.size() && "ステージ番号が範囲外です");
		return stageDatas_[index];
	}

	void SetCurrentStageIndex(int32_t index)
	{ 
		// indexが0以上、かつstageDatas_の要素数より小さいことを確認する
		assert(index >= 0 && index < stageDatas_.size() && "ステージ番号が範囲外です");
		currentStageIndex_ = index;
	}

	int32_t GetCurrentStageIndex() const { return currentStageIndex_; }

	/// <summary>
	/// 現在ステージのステージデータ取得
	/// </summary>
	/// <param name="index">ステージ番号</param>
	/// <returns>ステージデータ</returns>
	const StageData& GetCurrentStageData() const { return GetStageData(currentStageIndex_); }

	/// <summary>
	/// ステージ名指定で現在ステージ番号設定
	/// </summary>
	/// <param name="index">ステージ番号</param>
	void SetCurrentStageIndexByName(const std::string& name);

	/// <summary>
	/// ステージ数取得
	/// </summary>
	/// <returns></returns>
	int32_t GetStageCount() const { return static_cast<int32_t>(stageDatas_.size()); }

	/// <summary>
	/// 指定したステージをクリア済みにする
	/// </summary>
	void MarkStageCleared(int32_t index);

	/// <summary>
	/// 現在選択中のステージをクリア済みにする
	/// </summary>
	void MarkCurrentStageCleared() { MarkStageCleared(currentStageIndex_); }

	/// <summary>
	/// 指定したステージがクリア済みか調べる
	/// </summary>
	bool IsStageCleared(int32_t index) const;

	/// <summary>
	/// 全ステージをクリア済みか調べる
	/// </summary>
	bool AreAllStagesCleared() const;
	
private:
	// 全ステージデータ
	std::vector<StageData> stageDatas_;

	// ステージごとのクリア状態。同じゲームを起動している間は維持する。
	std::vector<bool> clearedStages_;

	// 現在のステージ番号
	int32_t currentStageIndex_ = 0;

};
