#pragma once
#include "framework.h"
#include "GraphicEngine.h"

struct CBSsr
{
	DirectX::XMFLOAT4X4 gView;
	DirectX::XMFLOAT4X4 gViewProj;
	DirectX::XMFLOAT4X4 Proj;
	XMFLOAT3   EyePosW;
	//XMFLOAT2 Dimensions;
	//float FarClip;
};

class PostProcess;

class Ssr
{
public:
	Ssr(UINT width, UINT height, float farPlane);
	void BuildSsrRootSignature();
	void CreateSsrPSO();
	void CreateSsrTex();
	void CreateSsrDescriptors();
	void ComputeSsr(ID3D12GraphicsCommandList* cmdList, PostProcess* postProcess);
	void UpdateSsrCB(const GameTimer& Timer);
	void Update(const GameTimer& Timer);
	void InitSsrCb(float farPlane);
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetSsrSrvGpuHandle();
	void SetNormalSrvIndex(int value) { mNormalSrvIndex = value; };
	void SetWPosSrvIndex(int value) { mWPosSrvIndex = value; };
	void InitPlane();

private:
	int mNormalSrvIndex;
	int mWPosSrvIndex;
	UINT mWidth;
	UINT mHeight;
	ComPtr<ID3D12Resource> mSsrMap = nullptr;
	static const DXGI_FORMAT SsrMapFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	int mSsrSrvIndex;
	int mSsrRtvIndex;

	ComPtr<ID3D12RootSignature> mSsrRootSignature;
	ComPtr<ID3D12PipelineState> mSsrPSO;
	unique_ptr< ConstantBuffer<CBSsr> > mCBSsr;
	CBSsr SsrCB;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	MeshInfo* mPlane;
};

