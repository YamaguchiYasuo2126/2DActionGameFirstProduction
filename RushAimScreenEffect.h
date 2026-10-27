#pragma once

#include "KamataEngine.h"
#include <d3d12.h>
#include <wrl.h>

// 突進方向の選択中に、画面全体へシェーダー演出を重ねる。
class RushAimScreenEffect final {
public:
	bool Initialize();
	void Draw(float strength, float elapsedSeconds, const KamataEngine::Vector2& centerUv) const;

private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
};
