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
	void DrawSceneToShadowMap();
	void UpdateShadowPassCB();
	void UpdateShadowTransform();
	void Update(const GameTimer& Timer);
	XMFLOAT4X4& GetShadowTransform() { return mShadowTransform; }


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

	unique_ptr< ConstantBuffer<CBPerPass> > PassCB;
	XMFLOAT4X4 mLightView = MathHelper::Identity4x4();
	XMFLOAT4X4 mLightProj = MathHelper::Identity4x4();
	XMFLOAT4X4 mShadowTransform = MathHelper::Identity4x4();
	CBPerPass mShadowPassCB;// index 1 of pass cbuffer.
	DirectX::BoundingSphere mSceneBounds;

	float mLightNearZ = 0.0f;
	float mLightFarZ = 0.0f;
	XMFLOAT3 mLightPosW;
	XMFLOAT3 mRotatedLightDirections[3];
	float mLightRotationAngle = 0.0f;
	XMFLOAT3 mBaseLightDirections[3] = {
		XMFLOAT3(0.57735f, -0.57735f, 0.57735f),
		XMFLOAT3(-0.57735f, -0.57735f, 0.57735f),
		XMFLOAT3(0.0f, -0.707f, -0.707f)
	};


};