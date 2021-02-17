#include "Ssr.h"
#include <DirectXPackedVector.h>
#include "PostProcess.h"

using namespace DirectX::PackedVector;

Ssr::Ssr(UINT width, UINT height, float farPlane)
{
	mWidth = width;
	mHeight = height;

	mViewport.TopLeftX = 0.0f;
	mViewport.TopLeftY = 0.0f;
	mViewport.Width = mWidth;
	mViewport.Height = mHeight;
	mViewport.MinDepth = 0.0f;
	mViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, (long)mWidth, (long)mHeight };

	CreateSsrTex();
	CreateSsrDescriptors();

	BuildSsrRootSignature();
	CreateSsrPSO();

	InitSsrCb(farPlane);
}

void Ssr::Update(const GameTimer& Timer)
{
	UpdateSsrCB(Timer);
}

void Ssr::CreateSsrPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC SsrPsoDesc;

	ZeroMemory(&SsrPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	ShaderState* shader = GetEngine()->GetShader();
	ComPtr<ID3DBlob> vs = shader->CreateVSShader(L"Shader\\Ssr.hlsl");
	ComPtr<ID3DBlob> ps = shader->CreatePSShader(L"Shader\\Ssr.hlsl");
	SsrPsoDesc.InputLayout = { nullptr, 0 };
	SsrPsoDesc.pRootSignature = mSsrRootSignature.Get();
	SsrPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(vs->GetBufferPointer()),
		vs->GetBufferSize()
	};
	SsrPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(ps->GetBufferPointer()),
		ps->GetBufferSize()
	};

	SsrPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	SsrPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	SsrPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	SsrPsoDesc.SampleMask = UINT_MAX;
	SsrPsoDesc.NumRenderTargets = 1;
	SsrPsoDesc.DepthStencilState.DepthEnable = false;
	SsrPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	SsrPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	SsrPsoDesc.RTVFormats[0] = SsrMapFormat;
	SsrPsoDesc.SampleDesc.Count = 1;
	SsrPsoDesc.SampleDesc.Quality = 0;
	SsrPsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	ThrowIfFailed(GetEngine()->GetDevice()->CreateGraphicsPipelineState(&SsrPsoDesc, IID_PPV_ARGS(&mSsrPSO)));
}

void Ssr::BuildSsrRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable0;
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE texTable1;
	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE texTable2;
	texTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[2].InitAsDescriptorTable(1, &texTable2, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[3].InitAsConstantBufferView(0);

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC depthMapSam(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,
		0,
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE);

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	std::array<CD3DX12_STATIC_SAMPLER_DESC, 4> staticSamplers =
	{
		pointClamp, linearClamp, depthMapSam, linearWrap
	};

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(GetEngine()->GetDevice()->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mSsrRootSignature.GetAddressOf())));
}

void Ssr::ComputeSsr(ID3D12GraphicsCommandList* cmdList, PostProcess* postProcess)
{
	cmdList->RSSetViewports(1, &mViewport);
	cmdList->RSSetScissorRects(1, &mScissorRect);

	// We compute the initial Ssr to AmbientMap0.

	// Change to RENDER_TARGET.
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mSsrMap.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	float clearValue[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	CD3DX12_CPU_DESCRIPTOR_HANDLE SsrHandle = GetEngine()->GetDescriptorHeap()->GetRtvDescriptorCpuHandle(mSsrRtvIndex);
	cmdList->ClearRenderTargetView(SsrHandle, clearValue, 0, nullptr);

	// Specify the buffers we are going to render to.
	cmdList->OMSetRenderTargets(1, &SsrHandle, true, nullptr);

	// Bind the constant buffer for this pass.
	cmdList->SetGraphicsRootSignature(mSsrRootSignature.Get());
	auto SsrCBAddress = mCBSsr->Resource()->GetGPUVirtualAddress();
	cmdList->SetGraphicsRootConstantBufferView(3, SsrCBAddress);

	cmdList->SetPipelineState(mSsrPSO.Get());

	// Draw fullscreen quad.
	cmdList->IASetVertexBuffers(0, 0, nullptr);
	cmdList->IASetIndexBuffer(nullptr);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(6, 1, 0, 0);

	// Change back to GENERIC_READ so we can read the texture in a shader.
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mSsrMap.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));

}

void Ssr::CreateSsrTex()
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	// Ambient occlusion maps are at half resolution.
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	texDesc.Format = SsrMapFormat;

	float ambientClearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	D3D12_CLEAR_VALUE optClear = CD3DX12_CLEAR_VALUE(SsrMapFormat, ambientClearColor);

	ThrowIfFailed(GetEngine()->GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&mSsrMap)));
}

void Ssr::CreateSsrDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	srvDesc.Format = SsrMapFormat;
	GetEngine()->GetDevice()->CreateShaderResourceView(mSsrMap.Get(), &srvDesc, GetEngine()->GetDescriptorHeap()->GetSrvDescriptorCpuHandle());
	mSsrSrvIndex = GetEngine()->GetDescriptorHeap()->GetSrvDescriptorIndex();

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;
	rtvDesc.Format = SsrMapFormat;

	GetEngine()->GetDevice()->CreateRenderTargetView(mSsrMap.Get(), &rtvDesc, GetEngine()->GetDescriptorHeap()->GetRtvDescriptorCpuHandle());
	mSsrRtvIndex = GetEngine()->GetDescriptorHeap()->GetRtvDescriptorIndex();
}

void Ssr::InitSsrCb(float farPlane)
{
	mCBSsr = make_unique<ConstantBuffer<CBSsr>>(GetEngine()->GetDevice(), 1, true);

	SsrCB.FarClip = farPlane;
	SsrCB.Dimensions = { (float)mWidth, (float)mHeight };
}

CD3DX12_GPU_DESCRIPTOR_HANDLE Ssr::GetSsrSrvGpuHandle()
{
	return GetEngine()->GetDescriptorHeap()->GetSrvDescriptorGpuHandle(mSsrSrvIndex); 
}

void Ssr::UpdateSsrCB(const GameTimer& Timer)
{
	mCBSsr->Update();

	XMMATRIX view = GetEngine()->GetView();
	XMMATRIX proj = GetEngine()->GetProj();

	XMStoreFloat4x4(&SsrCB.gView, XMMatrixTranspose(view));
	XMStoreFloat4x4(&SsrCB.Proj, XMMatrixTranspose(proj));

	mCBSsr->Update(0, SsrCB);
}


