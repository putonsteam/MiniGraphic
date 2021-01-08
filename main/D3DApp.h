#pragma once
#include "framework.h"
#include "ShaderState.h"
#include "GameTimer.h"
#include "ConstantBuffer.h"
#include "FrameResource.h"
#include "RenderItem.h"
#include "Lighting.h"
#include "Sky.h"

class D3DApp
{
public:
	D3DApp() {}
	bool Init(int Width, int Height, HWND wnd);
	void Run();
	void Draw(const GameTimer& Timer);
	void Update(const GameTimer& Timer);
	void BuildPSO(const wchar_t* vsFile, const wchar_t* psFile);

private:
	array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();
	void LoadRenderItem();

	FrameResource* mCurrFrameResource;
	float mLightRotationAngle = 0.0f;
	XMFLOAT3 mBaseLightDirections[3] = {
		XMFLOAT3(0.57735f, -0.57735f, 0.57735f),
		XMFLOAT3(-0.57735f, -0.57735f, 0.57735f),
		XMFLOAT3(0.0f, -0.707f, -0.707f)
	};
	XMFLOAT3 mRotatedLightDirections[3];
	CD3DX12_GPU_DESCRIPTOR_HANDLE mNullSrv;
	ComPtr<ID3D12PipelineState> mBasePSO;

	Sky mSky;
};


