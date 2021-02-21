#pragma once
#include "framework.h"
#include "GraphicEngine.h"

struct CBSsao
{
	DirectX::XMFLOAT4X4 gView = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 Proj;
	DirectX::XMFLOAT4X4 InvProj;
	DirectX::XMFLOAT4X4 ProjTex;
	DirectX::XMFLOAT4   OffsetVectors[14];

	// Coordinates given in view space.
	float OcclusionRadius = 0.5f;
	float OcclusionFadeStart = 0.2f;
	float OcclusionFadeEnd = 2.0f;
	float SurfaceEpsilon = 0.05f;
};

class PostProcess;

class Ssao
{
public:
	Ssao(UINT width, UINT height);
	void CreateDepthDescriptors();
	void BuildSsaoRootSignature();
	void CreateSsaoPSO();
	void CreateSsaoTex();
	void CreateSsaoDescriptors();
	void CreateRandomVectorTexture();
	void CreateRandomDescriptors();
	void ComputeSsao(ID3D12GraphicsCommandList* cmdList, PostProcess* postProcess);
	void UpdateSsaoCB(const GameTimer& Timer);
	void Update(const GameTimer& Timer);
	void BuildOffsetVectors();
	void InitSsaoCb();
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetSsaoSrvGpuHandle();
	void SetNormalSrvIndex(int value) { mNormalSrvIndex = value; };
	void SetWPosSrvIndex(int value) { mWPosSrvIndex = value; };

private:
	UINT mWidth;
	UINT mHeight;
	ComPtr<ID3D12Resource> mSsaoMap = nullptr;
	ComPtr<ID3D12Resource> mRandomVectorMap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mRandomVectorMapUploadBuffer;
	static const DXGI_FORMAT AmbientMapFormat = DXGI_FORMAT_R16_UNORM;
	int mRandomVectorSrvIndex;
	int mSsaoSrvIndex;
	int mSsaoRtvIndex;
	int mDepthSrvIndex;
	int mNormalSrvIndex;
	int mWPosSrvIndex;

	ComPtr<ID3D12RootSignature> mSsaoRootSignature;
	ComPtr<ID3D12PipelineState> mSsaoPSO;
	unique_ptr< ConstantBuffer<CBSsao> > mCBSsao;
	CBSsao ssaoCB;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;
};

