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
#include "DeferredShading.h"

class D3DApp
{
public:
	D3DApp();
	bool Init(int Width, int Height, HWND wnd);
	void Run();
	void Draw(const GameTimer& Timer);
	void Update(const GameTimer& Timer);
	void UpdateFeatureCB(const GameTimer& Timer);

private:
	void LoadRenderItem();

	FrameResource* mCurrFrameResource;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mNullSrv;
	CBFeature mFeatureCB;  // index 0 of pass cbuffer.
	unique_ptr< ConstantBuffer<CBFeature> > mCBFeature = nullptr;
	Sky mSky;
	ShadowMap* mShadowMap;
	Ssao* mSsao;
	DeferredShading* m_DeferredShading;
};


