#pragma once
#include "framework.h"
#include "GraphicEngine.h"

struct SsaoConstants
{
	DirectX::XMFLOAT4X4 Proj;
	DirectX::XMFLOAT4X4 InvProj;
	DirectX::XMFLOAT4X4 ProjTex;
	DirectX::XMFLOAT4   OffsetVectors[14];

	DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };

	// Coordinates given in view space.
	float OcclusionRadius = 0.5f;
	float OcclusionFadeStart = 0.2f;
	float OcclusionFadeEnd = 2.0f;
	float SurfaceEpsilon = 0.05f;
};

class Ssao
{
public:
	Ssao(UINT width, UINT height);
	void CreateNormalTex();
	void CreateNormalDescriptors();
	void DrawNormalsAndDepth(ID3D12GraphicsCommandList* cmdList);
	void CreateNormalDepthPSO();
	void BuildSsaoRootSignature();
	void CreateSsaoPSO();
	void CreateSsaoTex();
	void CreateSsaoDescriptors();
	void CreateRandomVectorTexture();
	void CreateRandomDescriptors();
	void ComputeSsao(ID3D12GraphicsCommandList* cmdList);
	void UpdateSsaoCB(const GameTimer& Timer);
	void Update(const GameTimer& Timer);
	void BuildOffsetVectors();
	void InitSsaoCb();

private:
	UINT mWidth;
	UINT mHeight;
	ComPtr<ID3D12Resource> mNormalMap = nullptr;
	ComPtr<ID3D12Resource> mSsaoMap = nullptr;
	ComPtr<ID3D12Resource> mRandomVectorMap = nullptr;
	static const DXGI_FORMAT NormalMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	static const DXGI_FORMAT AmbientMapFormat = DXGI_FORMAT_R16_UNORM;
	int mNormalRtvIndex;
	int mNormalSrvIndex;
	int mRandomVectorSrvIndex;
	int mSsaoSrvIndex;
	int mSsaoRtvIndex;
	ComPtr<ID3D12RootSignature> mSsaoRootSignature;
	ComPtr<ID3D12PipelineState> mNormalDepPSO;
	ComPtr<ID3D12PipelineState> mSsaoPSO;
	unique_ptr< ConstantBuffer<SsaoConstants> > SsaoCb;
	SsaoConstants ssaoCB;
};

