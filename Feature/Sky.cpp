#include "Sky.h"

CD3DX12_GPU_DESCRIPTOR_HANDLE Sky::GetSkyHeapStart()
{
	return GetEngine()->GetDescriptorHeap()->GetSrvDescriptorGpuHandle(mSkyTexHeapIndex);
}

void Sky::LoadRenderItem()
{
	auto sky = std::make_unique<LoadMaterial>();
	sky->Name = "sky";
	sky->MatCBIndex = 4;
	sky->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	sky->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	sky->Roughness = 1.0f;
	//sky->SetDiffuseSrv(L"source/Textures/grasscube1024.dds");
	// 
	// 	//GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
	// 
	auto sphere = std::make_unique<MeshInfo>();
	// 
	sphere->CreateSphere(0.5f, 20, 20);
	// 	//skullRitem->Geo = move(sky);
	// 
	auto skyRitem = std::make_unique<RenderItem>();
	skyRitem->IndexCount = sphere->IndexCount;
	// 
	XMStoreFloat4x4(&skyRitem->World, XMMatrixScaling(5000.0f, 5000.0f, 5000.0f));
	skyRitem->TexTransform = MathHelper::Identity4x4();
	skyRitem->ObjCBIndex = 4;
	skyRitem->Mat = move(sky);
	skyRitem->Geo = move(sphere);
	skyRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	//skyRitem->StartIndexLocation = skyRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
	//skyRitem->BaseVertexLocation = skyRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
	//mRitemLayer[(int)RenderLayer::Sky].push_back(move(skyRitem));
	GetEngine()->AddRenderItem(RenderLayer::Sky, skyRitem);

	//BuildBaseRootSignature();

	//BuildPSO(L"Shader\\Default.hlsl", L"Shader\\Default.hlsl");
	BuildSkyPSO(L"Shader\\Sky.hlsl", L"Shader\\Sky.hlsl");
	mSkyTexHeapIndex = GetEngine()->GetTextureList()->LoadCure(L"source/Textures/grasscube1024.dds");
}

void Sky::BuildSkyPSO(const wchar_t* vsFile, const wchar_t* psFile)
{
	//
		// PSO for sky.
		//
	ShaderState* shader = GetEngine()->GetShader();
	ComPtr<ID3DBlob> vs = shader->CreateVSShader(vsFile);
	ComPtr<ID3DBlob> ps = shader->CreatePSShader(psFile);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc;

	//
	// PSO for opaque objects.
	//
	ZeroMemory(&skyPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	skyPsoDesc.InputLayout = { shader->GetLayout().data(), (UINT)shader->GetLayout().size() };
	skyPsoDesc.pRootSignature = GetEngine()->GetBaseRootSignature();
	skyPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(vs->GetBufferPointer()),
		vs->GetBufferSize()
	};
	skyPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(ps->GetBufferPointer()),
		ps->GetBufferSize()
	};
	skyPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	skyPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	skyPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	skyPsoDesc.SampleMask = UINT_MAX;
	skyPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	skyPsoDesc.NumRenderTargets = 1;
	skyPsoDesc.RTVFormats[0] = GetEngine()->mBackBufferFormat;
	skyPsoDesc.SampleDesc.Count = /*m4xMsaaState ? 4 : */1;
	skyPsoDesc.SampleDesc.Quality = /*m4xMsaaState ? (m4xMsaaQuality - 1) : */0;
	skyPsoDesc.DSVFormat = GetEngine()->mDepthStencilFormat;
	//D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc = opaquePsoDesc;

	// The camera is inside the sky sphere, so just turn off culling.
	skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	// Make sure the depth function is LESS_EQUAL and not just LESS.  
	// Otherwise, the normalized depth values at z = 1 (NDC) will 
	// fail the depth test if the depth buffer was cleared to 1.
	skyPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	ThrowIfFailed(GetEngine()->GetDevice()->CreateGraphicsPipelineState(&skyPsoDesc, IID_PPV_ARGS(&mSkyPSO)));

}

void Sky::Draw(const GameTimer& Timer)
{
	auto mCommandList = GetEngine()->GetCommandList();
	mCommandList->SetPipelineState(mSkyPSO.Get());
	mCommandList->OMSetRenderTargets(1, &GetEngine()->CurrentBackBufferView(), true, &GetEngine()->DepthStencilView());
	GetEngine()->DrawRenderItems(RenderLayer::Sky);
}
