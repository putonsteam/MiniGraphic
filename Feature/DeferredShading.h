#pragma once
#include "framework.h"

enum GBufferType
{
	Diffuse,
	Pos,
	Normal,
	Material,
	FeatureAttr,
	Total,
};

#define BUFFER_COUNT Total

class DeferredShading
{
public:
	DeferredShading(int Width, int Height);
	void CreateGBufferTexture();
	void CreateDeferredTexture();
	void CreateGbufferView();
	void CreateDeferredView();
	void CreateGBufferPSO();
	void RenderGBuffer(ID3D12GraphicsCommandList* cmdList);
	void BuildPSO(const wchar_t* vsFile, const wchar_t* psFile);
	void Render(ID3D12GraphicsCommandList* mCommandList);
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetGBufferSrvGpuHandle();
	int GetGBufferSrv(GBufferType type) { return mGBufferSrv[type]; }
	ID3D12Resource* GetDeferredResource() { return mDeferredTex.Get(); }
	int GetDeferredSrv() { return mDeferredSrv; }

private:
	int mTextureWidth;
	int mTextureHeight;
	const DXGI_FORMAT mGbufferFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
	ComPtr<ID3D12Resource> mGBufferArray[BUFFER_COUNT];
	ComPtr<ID3D12Resource> mDeferredTex;
	int mGBufferRtv[BUFFER_COUNT];
	int mGBufferSrv[BUFFER_COUNT];
	int mDeferredRtv;
	int mDeferredSrv;

	ComPtr<ID3D12PipelineState> mGBufferPSO;
	ComPtr<ID3D12PipelineState> mBasePSO;


};