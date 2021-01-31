#include "DeferredShading.h"
#include "GraphicEngine.h"

DeferredShading::DeferredShading(int Width, int Height)
{
	mTextureWidth = Width;
	mTextureHeight = Height;
	CreateGBufferTexture();
	CreateGbufferView();
	CreateGBufferPSO();
	BuildPSO(L"Shader\\Default.hlsl", L"Shader\\Default.hlsl");
}

void DeferredShading::CreateGBufferTexture()
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mTextureWidth;
	texDesc.Height = mTextureHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = mGbufferFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	float ClearColor[] = { 0.0f, 0.0f, 1.0f, 0.0f };
	CD3DX12_CLEAR_VALUE optClear(mGbufferFormat, ClearColor);
	for (int i = 0; i < BUFFER_COUNT; ++i)
	{
		ThrowIfFailed(GetEngine()->GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			&optClear,
			IID_PPV_ARGS(&mGBufferArray[i])));
	}
}

void DeferredShading::CreateGbufferView()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = mGbufferFormat;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	for (int i = 0; i < BUFFER_COUNT; ++i)
	{

		GetEngine()->GetDevice()->CreateShaderResourceView(mGBufferArray[i].Get(), &srvDesc,
			GetEngine()->GetDescriptorHeap()->GetSrvDescriptorCpuHandle());
		mGBufferSrv[i] = GetEngine()->GetDescriptorHeap()->GetSrvDescriptorIndex();
	}

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = mGbufferFormat;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;
	for (int i = 0; i < BUFFER_COUNT; ++i)
	{
		GetEngine()->GetDevice()->CreateRenderTargetView(mGBufferArray[i].Get(), &rtvDesc,
			GetEngine()->GetDescriptorHeap()->GetRtvDescriptorCpuHandle());

		mGBufferRtv[i] = GetEngine()->GetDescriptorHeap()->GetRtvDescriptorIndex();
	}
}

void DeferredShading::RenderGBuffer(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->RSSetViewports(1, GetEngine()->GetViewport());
	cmdList->RSSetScissorRects(1, GetEngine()->GetScissor());
	float clearValue[] = { 0.0f, 0.0f, 1.0f, 0.0f };
	for (int i = 0; i < BUFFER_COUNT; ++i)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE GBufferView = GetEngine()->GetDescriptorHeap()->GetRtvDescriptorCpuHandle(mGBufferRtv[i]);
		cmdList->ClearRenderTargetView(GBufferView, clearValue, 0, nullptr);

		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mGBufferArray[i].Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
	}

	// Clear the screen normal map and depth buffer.
	D3D12_CPU_DESCRIPTOR_HANDLE GBufferView = GetEngine()->GetDescriptorHeap()->GetRtvDescriptorCpuHandle(mGBufferRtv[0]);

	cmdList->ClearDepthStencilView(GetEngine()->DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	cmdList->OMSetRenderTargets(BUFFER_COUNT, &GBufferView, true, &GetEngine()->DepthStencilView());

	cmdList->SetPipelineState(mGBufferPSO.Get());

	GetEngine()->DrawRenderItems(RenderLayer::Opaque);

	for (int i = 0; i < BUFFER_COUNT; ++i)
	{
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mGBufferArray[i].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	}
}

void DeferredShading::CreateGBufferPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC GbufferPsoDesc;
	ZeroMemory(&GbufferPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	ShaderState* shader = GetEngine()->GetShader();
	ComPtr<ID3DBlob> vs = shader->CreateVSShader(L"Shader\\GBufferShader.hlsl");
	ComPtr<ID3DBlob> ps = shader->CreatePSShader(L"Shader\\GBufferShader.hlsl");
	GbufferPsoDesc.InputLayout = { shader->GetLayout().data(), (UINT)shader->GetLayout().size() };
	GbufferPsoDesc.pRootSignature = GetEngine()->GetBaseRootSignature();
	GbufferPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(vs->GetBufferPointer()),
		vs->GetBufferSize()
	};
	GbufferPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(ps->GetBufferPointer()),
		ps->GetBufferSize()
	};

	GbufferPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	GbufferPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	GbufferPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	GbufferPsoDesc.SampleMask = UINT_MAX;
	GbufferPsoDesc.NumRenderTargets = 1;
	GbufferPsoDesc.DepthStencilState.DepthEnable = false;
	GbufferPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	GbufferPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GbufferPsoDesc.RTVFormats[0] = mGbufferFormat;
	GbufferPsoDesc.SampleDesc.Count = 1;
	GbufferPsoDesc.SampleDesc.Quality = 0;
	GbufferPsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	ThrowIfFailed(GetEngine()->GetDevice()->CreateGraphicsPipelineState(&GbufferPsoDesc, IID_PPV_ARGS(&mGBufferPSO)));
}

void DeferredShading::BuildPSO(const wchar_t* vsFile, const wchar_t* psFile)
{
	ShaderState* shader = GetEngine()->GetShader();
	ComPtr<ID3DBlob> vs = shader->CreateVSShader(vsFile);
	ComPtr<ID3DBlob> ps = shader->CreatePSShader(psFile);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { nullptr, 0 };
	opaquePsoDesc.pRootSignature = GetEngine()->GetBaseRootSignature();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(vs->GetBufferPointer()),
		vs->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(ps->GetBufferPointer()),
		ps->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = GetEngine()->mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = /*m4xMsaaState ? 4 : */1;
	opaquePsoDesc.SampleDesc.Quality = /*m4xMsaaState ? (m4xMsaaQuality - 1) : */0;
	opaquePsoDesc.DSVFormat = GetEngine()->mDepthStencilFormat;
	ThrowIfFailed(GetEngine()->GetDevice()->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mBasePSO)));
}

void DeferredShading::Render(ID3D12GraphicsCommandList* mCommandList)
{
	mCommandList->SetPipelineState(mBasePSO.Get());

	mCommandList->RSSetViewports(1, GetEngine()->GetViewport());
	mCommandList->RSSetScissorRects(1, GetEngine()->GetScissor());

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(GetEngine()->CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(GetEngine()->DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &GetEngine()->CurrentBackBufferView(), true, &GetEngine()->DepthStencilView());

	mCommandList->IASetVertexBuffers(0, 0, nullptr);
	mCommandList->IASetIndexBuffer(nullptr);
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCommandList->DrawInstanced(6, 1, 0, 0);

}

CD3DX12_GPU_DESCRIPTOR_HANDLE DeferredShading::GetGBufferSrvGpuHandle()
{
	return GetEngine()->GetDescriptorHeap()->GetSrvDescriptorGpuHandle(mGBufferSrv[0]);
}
