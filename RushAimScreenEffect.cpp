#include "RushAimScreenEffect.h"

#include "base/DirectXCommon.h"
#include <algorithm>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

using namespace KamataEngine;
using Microsoft::WRL::ComPtr;

namespace {
struct ShaderParameters {
	float centerX;
	float centerY;
	float strength;
	float elapsedSeconds;
	float screenWidth;
	float screenHeight;
	float padding[2];
};
static_assert(sizeof(ShaderParameters) == 8 * sizeof(float));

bool CompileShader(const char* entryPoint, const char* target, ComPtr<ID3DBlob>& result)
{
	ComPtr<ID3DBlob> errors;
	const HRESULT hr = D3DCompileFromFile(
		L"Resources/shaders/RushAimScreen.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entryPoint, target, 0, 0, result.GetAddressOf(), errors.GetAddressOf());
	if (FAILED(hr) && errors)
	{
		OutputDebugStringA(static_cast<const char*>(errors->GetBufferPointer()));
	}
	return SUCCEEDED(hr);
}
} // namespace

bool RushAimScreenEffect::Initialize()
{
	ID3D12Device* device = DirectXCommon::GetInstance()->GetDevice();
	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;
	if (!CompileShader("VSMain", "vs_5_0", vertexShader) ||
		!CompileShader("PSMain", "ps_5_0", pixelShader))
	{
		return false;
	}

	D3D12_ROOT_PARAMETER parameter{};
	parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	parameter.Constants.Num32BitValues = 8;
	parameter.Constants.ShaderRegister = 0;
	parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	D3D12_ROOT_SIGNATURE_DESC rootDescription{};
	rootDescription.NumParameters = 1;
	rootDescription.pParameters = &parameter;
	rootDescription.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	ComPtr<ID3DBlob> serializedRoot;
	ComPtr<ID3DBlob> rootErrors;
	if (FAILED(D3D12SerializeRootSignature(&rootDescription, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRoot.GetAddressOf(), rootErrors.GetAddressOf())))
	{
		if (rootErrors)
		{
			OutputDebugStringA(static_cast<const char*>(rootErrors->GetBufferPointer()));
		}
		return false;
	}
	if (FAILED(device->CreateRootSignature(0, serializedRoot->GetBufferPointer(),
		serializedRoot->GetBufferSize(), IID_PPV_ARGS(rootSignature_.GetAddressOf()))))
	{
		return false;
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline{};
	pipeline.pRootSignature = rootSignature_.Get();
	pipeline.VS = {vertexShader->GetBufferPointer(), vertexShader->GetBufferSize()};
	pipeline.PS = {pixelShader->GetBufferPointer(), pixelShader->GetBufferSize()};
	pipeline.BlendState.RenderTarget[0].BlendEnable = TRUE;
	pipeline.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	pipeline.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	pipeline.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	pipeline.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	pipeline.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	pipeline.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	pipeline.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	pipeline.SampleMask = UINT_MAX;
	pipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	pipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	pipeline.RasterizerState.DepthClipEnable = TRUE;
	pipeline.DepthStencilState.DepthEnable = FALSE;
	pipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	pipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	pipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipeline.NumRenderTargets = 1;
	pipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pipeline.SampleDesc.Count = 1;
	const HRESULT pipelineResult = device->CreateGraphicsPipelineState(&pipeline,
		IID_PPV_ARGS(pipelineState_.GetAddressOf()));
	if (FAILED(pipelineResult))
	{
		OutputDebugStringA("Rush aim screen pipeline creation failed.\n");
		return false;
	}
	return true;
}

void RushAimScreenEffect::Draw(float strength, float elapsedSeconds, const Vector2& centerUv) const
{
	if (!pipelineState_ || strength <= 0.001f)
	{
		return;
	}
	DirectXCommon* graphics = DirectXCommon::GetInstance();
	ID3D12GraphicsCommandList* commands = graphics->GetCommandList();
	const int32_t width = graphics->GetBackBufferWidth();
	const int32_t height = graphics->GetBackBufferHeight();
	if (width <= 0 || height <= 0)
	{
		return;
	}
	// パイプラインのRTV形式と合わせ、通常のUNORMビューを使う。
	graphics->SetRenderTargets(false);
	const D3D12_VIEWPORT viewport{0.0f, 0.0f, static_cast<float>(width),
		static_cast<float>(height), 0.0f, 1.0f};
	const D3D12_RECT scissor{0, 0, width, height};
	commands->RSSetViewports(1, &viewport);
	commands->RSSetScissorRects(1, &scissor);
	commands->SetGraphicsRootSignature(rootSignature_.Get());
	commands->SetPipelineState(pipelineState_.Get());
	commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	const ShaderParameters parameters{
		centerUv.x, centerUv.y, (std::clamp)(strength, 0.0f, 1.0f), elapsedSeconds,
		static_cast<float>(width), static_cast<float>(height), {0.0f, 0.0f}};
	commands->SetGraphicsRoot32BitConstants(0, 8, &parameters, 0);
	commands->DrawInstanced(3, 1, 0, 0);
}
