#include "PipelineState.h"
#include "GraphicEngine.h"

void PipelineState::BuildPSO(string name, const wchar_t* vsFile, const wchar_t* psFile, ComPtr<ID3D12RootSignature> signature)
{
	ComPtr<ID3DBlob> vs = mShaders.CreateVSShader(vsFile);
	ComPtr<ID3DBlob> ps = mShaders.CreatePSShader(psFile);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC basePsoDesc;

	ZeroMemory(&basePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	basePsoDesc.InputLayout = { mShaders.GetLayout().data(), (UINT)mShaders.GetLayout().size() };
	basePsoDesc.pRootSignature = signature.Get();
	basePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(vs->GetBufferPointer()),
		vs->GetBufferSize()
	};
	basePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(ps->GetBufferPointer()),
		ps->GetBufferSize()
	};
	basePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	basePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	basePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	basePsoDesc.SampleMask = UINT_MAX;
	basePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	basePsoDesc.NumRenderTargets = 1;
	basePsoDesc.RTVFormats[0] = GetEngine()->mBackBufferFormat;
	basePsoDesc.SampleDesc.Count = /*m4xMsaaState ? 4 : */1;
	basePsoDesc.SampleDesc.Quality = /*m4xMsaaState ? (m4xMsaaQuality - 1) : */0;
	basePsoDesc.DSVFormat = GetEngine()->mDepthStencilFormat;

	//
	// PSO for opaque objects.
	//

	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc = basePsoDesc;
	opaquePsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
	opaquePsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	ThrowIfFailed(GetEngine()->GetDevice()->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs[name])));
}