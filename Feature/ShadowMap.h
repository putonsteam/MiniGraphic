#pragma once
#include "framework.h"
#include "GraphicEngine.h"

class ShadowMap
{
public:
	ShadowMap(UINT width, UINT height);
	void CreateShadowMapTex();
	void CreateDescriptors();
	void CreatePSO();
	//void CrateRootSignature();
	void DrawSceneToShadowMap();

private:
	ComPtr<ID3D12Resource> mShadowMap = nullptr;
	ComPtr<ID3D12RootSignature> mShdowMapRootSignature;
	ComPtr<ID3D12PipelineState> mShadowMapPSO;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	UINT mWidth = 0;
	UINT mHeight = 0;
	
	int mShadowMapDsvIndex;
	int mShadowMapRsvIndex;
};