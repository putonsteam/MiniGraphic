#include "ShadowMap.h"

void ShadowMap::CreateShadowMapTex()
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = GetEngine()->mClientWidth;
	texDesc.Height = GetEngine()->mClientHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	ThrowIfFailed(GetEngine()->GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&mShadowMap)));
}

void ShadowMap::CreateDescriptors()
{
	// Create DSV to resource so we can render to the shadow map.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.Texture2D.MipSlice = 0;
	GetEngine()->GetDevice()->CreateDepthStencilView(mShadowMap.Get(), &dsvDesc, GetEngine()->GetDescriptorHeap()->GetDsvDescriptorCpuHandle());
	mShadowMapDsvIndex = GetEngine()->GetDescriptorHeap()->GetDsvDescriptorIndex();

	// Create SRV to resource so we can sample the shadow map in a shader program.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.PlaneSlice = 0;
	GetEngine()->GetDevice()->CreateShaderResourceView(mShadowMap.Get(), &srvDesc, GetEngine()->GetDescriptorHeap()->GetSrvDescriptorCpuHandle());
	mShadowMapRsvIndex = GetEngine()->GetDescriptorHeap()->GetSrvDescriptorIndex();
}

ShadowMap::ShadowMap(UINT width, UINT height)
{
	mWidth = width;
	mHeight = height;

	mViewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	mScissorRect = { 0, 0, (int)width, (int)height };
}

void ShadowMap::DrawSceneToShadowMap()
{
	ID3D12GraphicsCommandList* mCommandList = GetEngine()->GetCommandList();
	mCommandList->RSSetViewports(1, &mViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Change to DEPTH_WRITE.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	//UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearDepthStencilView(GetEngine()->GetDescriptorHeap()->GetDsvDescriptorCpuHandle(mShadowMapDsvIndex),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Set null render target because we are only going to draw to
	// depth buffer.  Setting a null render target will disable color writes.
	// Note the active PSO also must specify a render target count of 0.
	mCommandList->OMSetRenderTargets(0, nullptr, false, &GetEngine()->GetDescriptorHeap()->GetDsvDescriptorCpuHandle(mShadowMapDsvIndex));

	mCommandList->SetPipelineState(mShadowMapPSO.Get());

	GetEngine()->DrawRenderItems(RenderLayer::Opaque);

	// Change back to GENERIC_READ so we can read the texture in a shader.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap.Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));

}

void ShadowMap::CreatePSO()
{
	ShaderState* shader = GetEngine()->GetShader();
	ComPtr<ID3DBlob> vs = shader->CreateVSShader(L"Shaders\\Shadows.hlsl");
	ComPtr<ID3DBlob> ps = shader->CreatePSShader(L"Shaders\\Shadows.hlsl");
// 	mShaders["shadowVS"] = d3dUtil::CompileShader(L"Shaders\\Shadows.hlsl", nullptr, "VS", "vs_5_1");
// 	mShaders["shadowOpaquePS"] = d3dUtil::CompileShader(L"Shaders\\Shadows.hlsl", nullptr, "PS", "ps_5_1");
// 	mShaders["shadowAlphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\Shadows.hlsl", alphaTestDefines, "PS", "ps_5_1");


	//
	// PSO for opaque objects.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC smapPsoDesc;
	ZeroMemory(&smapPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	smapPsoDesc.InputLayout = { shader->GetLayout().data(), (UINT)shader->GetLayout().size() };
	smapPsoDesc.pRootSignature = GetEngine()->GetBaseRootSignature();
	smapPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	smapPsoDesc.RasterizerState.DepthBias = 100000;
	smapPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
	smapPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
	smapPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(vs->GetBufferPointer()),
		vs->GetBufferSize()
	};
	smapPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(ps->GetBufferPointer()),
		ps->GetBufferSize()
	};
	smapPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	smapPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	smapPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	smapPsoDesc.SampleMask = UINT_MAX;
	smapPsoDesc.SampleDesc.Count = /*m4xMsaaState ? 4 : */1;
	smapPsoDesc.SampleDesc.Quality = /*m4xMsaaState ? (m4xMsaaQuality - 1) : */0;
	smapPsoDesc.DSVFormat = GetEngine()->mDepthStencilFormat;
	// Shadow map pass does not have a render target.
	smapPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	smapPsoDesc.NumRenderTargets = 0;

	ThrowIfFailed(GetEngine()->GetDevice()->CreateGraphicsPipelineState(&smapPsoDesc, IID_PPV_ARGS(&mShadowMapPSO)));
}


