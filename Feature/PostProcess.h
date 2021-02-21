#pragma once
#include "framework.h"
#include "GameTimer.h"

class DeferredShading;
class Ssao;
class Ssr;

class PostProcess
{
public:
	PostProcess(int Width, int Height, float FarPlane);
	void CreatePostProcessTexture();
	void CreatePostProcessView();
	void Prepare(ID3D12GraphicsCommandList* cmdList, DeferredShading* deferred);
	void Render(ID3D12GraphicsCommandList* cmdList);
	void BindRootDescriptor(ID3D12GraphicsCommandList* cmdList);
	void Update(const GameTimer& Timer);
	void SetNormalSrvIndex(int value) { mNormalSrvIndex = value; };
	void SetWPosSrvIndex(int value) { mWPosSrvIndex = value; };
	void SetDeferredSrvIndex(int value) { mDeferredSrvIndex = value; };

private:
	int mNormalSrvIndex;
	int mWPosSrvIndex;
	int mDeferredSrvIndex;
	int mWidth;
	int mHeight;
	ComPtr<ID3D12Resource> mPostProcessTex;
	int mPostProcessRtv;
	int mPostProcessSrv;
	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;
	ComPtr<ID3D12RootSignature> mPostProcessRootSignature;
	Ssr* mSsr;
};