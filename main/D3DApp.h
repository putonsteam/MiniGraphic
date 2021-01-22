#pragma once
#include "framework.h"
#include "ShaderState.h"
#include "GameTimer.h"
#include "ConstantBuffer.h"
#include "FrameResource.h"
#include "RenderItem.h"
#include "Lighting.h"
#include "Sky.h"
#include "ShadowMap.h"
#include "Ssao.h"

class D3DApp
{
public:
	D3DApp() {}
	bool Init(int Width, int Height, HWND wnd);
	void Run();
	void Draw(const GameTimer& Timer);
	void Update(const GameTimer& Timer);
	void UpdateFeatureCB(const GameTimer& Timer);
	void BuildPSO(const wchar_t* vsFile, const wchar_t* psFile);

private:
	void LoadRenderItem();

	FrameResource* mCurrFrameResource;
// 	float mLightRotationAngle = 0.0f;
// 	XMFLOAT3 mBaseLightDirections[3] = {
// 		XMFLOAT3(0.57735f, -0.57735f, 0.57735f),
// 		XMFLOAT3(-0.57735f, -0.57735f, 0.57735f),
// 		XMFLOAT3(0.0f, -0.707f, -0.707f)
// 	};
	//XMFLOAT3 mRotatedLightDirections[3];
	CD3DX12_GPU_DESCRIPTOR_HANDLE mNullSrv;
	ComPtr<ID3D12PipelineState> mBasePSO;
	ConstantFeature mFeatureCB;  // index 0 of pass cbuffer.
	unique_ptr< ConstantBuffer<ConstantFeature> > FeatureCB;
	Sky mSky;
	ShadowMap* mShadowMap;
	Ssao* mSsao;
};


