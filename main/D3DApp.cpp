#include "D3DApp.h"
#include "MeshInfo.h"
#include "GraphicEngine.h"
#include "LoadTexture.h"
#include "LoadMaterial.h"


bool D3DApp::Init(int Width, int Height, HWND wnd)
{
	GetEngine()->Init(Width, Height, wnd, D3D_FEATURE_LEVEL_11_0);

	GetEngine()->SetPosition(0.0f, 2.0f, -15.0f);

	LoadRenderItem();
	GetEngine()->SendCommandAndFulsh();
	return true;
}

void D3DApp::LoadRenderItem()
{
	GetEngine()->InitDescriptorHeap(10);

	auto skull = std::make_unique<MeshInfo>();
	auto skullRitem = std::make_unique<RenderItem>();

	skull->LoadTextMesh("source/Models/skull.txt");
	skullRitem->IndexCount = skull->IndexCount;
	skullRitem->Geo = move(skull);

	auto skullMat = std::make_unique<LoadMaterial>();
	skullMat->Name = "skullMat";
	skullMat->MatCBIndex = 0;
	skullMat->DiffuseAlbedo = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	skullMat->FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
	skullMat->Roughness = 0.2f;
	//skullMat->DiffuseSrvHeapIndex = 4;
	//skullMat->NormalSrvHeapIndex = 5;
	skullMat->SetDiffuseSrv(L"source/Textures/white1x1.dds");
	//skullMat->SetNormaSrv(L"source/Textures/default_nmap.dds");
	skullRitem->Mat = move(skullMat);

	XMStoreFloat4x4(&skullRitem->World, XMMatrixScaling(0.4f, 0.4f, 0.4f)*XMMatrixTranslation(0.0f, 1.0f, 0.0f));
	skullRitem->TexTransform = MathHelper::Identity4x4();
	skullRitem->ObjCBIndex = 0;
	skullRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//	skullRitem->StartIndexLocation = skull->StartIndexLocation;
// 	skullRitem->BaseVertexLocation = skull->BaseVertexLocation;

	//mRitemLayer[(int)RenderLayer::Opaque].push_back(move(skullRitem));
	GetEngine()->AddRenderItem(RenderLayer::Opaque, skullRitem);

	mSky.LoadRenderItem();
// 	auto sky = std::make_unique<LoadMaterial>();
// 	sky->Name = "sky";
// 	sky->MatCBIndex = 0;
// 	sky->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
// 	sky->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
// 	sky->Roughness = 1.0f;
// 	sky->SetDiffuseSrv(L"source/Textures/grasscube1024.dds");
// // 
// // 	//GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
// // 
//  	auto sphere = std::make_unique<MeshInfo>();
// // 
// 	sphere->CreateSphere(0.5f, 20, 20);
// // 	//skullRitem->Geo = move(sky);
// // 
//  	auto skyRitem = std::make_unique<RenderItem>();
//  	skyRitem->IndexCount = sphere->IndexCount;
// // 
// 	XMStoreFloat4x4(&skyRitem->World, XMMatrixScaling(5000.0f, 5000.0f, 5000.0f));
// 	skyRitem->TexTransform = MathHelper::Identity4x4();
// 	skyRitem->ObjCBIndex = 0;
// 	skyRitem->Mat = move(sky);
//  	skyRitem->Geo = move(sphere);
//  	skyRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
// 	//skyRitem->StartIndexLocation = skyRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
// 	//skyRitem->BaseVertexLocation = skyRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
// 	//mRitemLayer[(int)RenderLayer::Sky].push_back(move(skyRitem));
// 	GetEngine()->AddRenderItem(RenderLayer::Sky, skyRitem);

	BuildBaseRootSignature();

	BuildPSO(L"Shader\\Default.hlsl", L"Shader\\Default.hlsl");
	//BuildSkyPSO(L"Shader\\Sky.hlsl", L"Shader\\Sky.hlsl");

}

void D3DApp::BuildPSO(const wchar_t* vsFile, const wchar_t* psFile)
{
	ShaderState* shader = GetEngine()->GetShader();
	ComPtr<ID3DBlob> vs = shader->CreateVSShader(vsFile);
	ComPtr<ID3DBlob> ps = shader->CreatePSShader(psFile);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { shader->GetLayout().data(), (UINT)shader->GetLayout().size() };
	opaquePsoDesc.pRootSignature = mBaseRootSignature.Get();
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

array<const CD3DX12_STATIC_SAMPLER_DESC, 6> D3DApp::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}

void D3DApp::Run()
{
	GetEngine()->Run();
 	Update(GetEngine()->GetTimer());
 	Draw(GetEngine()->GetTimer());
}

void D3DApp::Update(const GameTimer& Timer)
{
	// Cycle through the circular frame resource array.
	mCurrFrameResource = GetEngine()->GetFrameResource();
	mCurrFrameResource->Update();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (mCurrFrameResource->GetFence() != 0 && GetEngine()->GetFence()->GetCompletedValue() < mCurrFrameResource->GetFence())
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(GetEngine()->GetFence()->SetEventOnCompletion(mCurrFrameResource->GetFence(), eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	//
	// Animate the lights (and hence shadows).
	//

	mLightRotationAngle += 0.1f*Timer.DeltaTime();

	XMMATRIX R = XMMatrixRotationY(mLightRotationAngle);
	for (int i = 0; i < 3; ++i)
	{
		XMVECTOR lightDir = XMLoadFloat3(&mBaseLightDirections[i]);
		lightDir = XMVector3TransformNormal(lightDir, R);
		XMStoreFloat3(&mRotatedLightDirections[i], lightDir);
	}

	//AnimateMaterials(Timer);
	//UpdateObjectCBs(Timer);
	//UpdateMaterialBuffer(Timer);
	//UpdateShadowTransform(Timer);
	//UpdateMainPassCB(Timer);
	//UpdateShadowPassCB(Timer);
	//UpdateSsaoCB(Timer);
	//GetEngine()->UpdateShaderParameter(Timer);

}

void D3DApp::Draw(const GameTimer& Timer)
{
	ComPtr<ID3D12CommandAllocator> cmdListAlloc = mCurrFrameResource->GetCurrentCommandAllocator();
	auto mCommandList = GetEngine()->GetCommandList();

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mBasePSO.Get()));

	mCommandList->RSSetViewports(1, GetEngine()->GetViewport());
	mCommandList->RSSetScissorRects(1, GetEngine()->GetScissor());

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetEngine()->CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(GetEngine()->CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(GetEngine()->DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &GetEngine()->CurrentBackBufferView(), true, &GetEngine()->DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { GetEngine()->GetSrvDescHeap() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mBaseRootSignature.Get());

	//auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(1, PassCB->Resource()->GetGPUVirtualAddress());

	// Bind all the materials used in this scene.  For structured buffers, we can bypass the heap and 
	// set as a root descriptor.
	//auto matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
	mCommandList->SetGraphicsRootShaderResourceView(2, MaterialBuffer->Resource()->GetGPUVirtualAddress());

	// Bind the sky cube map.  For our demos, we just use one "world" cube map representing the environment
	// from far away, so all objects will use the same cube map and we only need to set it once per-frame.  
	// If we wanted to use "local" cube maps, we would have to change them per-object, or dynamically
	// index into an array of cube maps.

	//CD3DX12_GPU_DESCRIPTOR_HANDLE skyTexDescriptor(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	//skyTexDescriptor.Offset(mSkyTexHeapIndex, mCbvSrvDescriptorSize);
	//CD3DX12_GPU_DESCRIPTOR_HANDLE mNullSrv;
	mCommandList->SetGraphicsRootDescriptorTable(3, msky.GetSkyHeapIndex());

	// Bind all the textures used in this scene.  Observe
	// that we only have to specify the first descriptor in the table.  
	// The root signature knows how many descriptors are expected in the table.
	mCommandList->SetGraphicsRootDescriptorTable(4, GetEngine()->GetSrvDescHeap()->GetGPUDescriptorHandleForHeapStart());

	GetEngine()->DrawRenderItems(RenderLayer::Opaque/*mCommandList, *//*mRitemLayer[(int)RenderLayer::Opaque]*/);

	mSky.Draw(Timer);
	//mCommandList->SetPipelineState(mSkyPSO.Get());
	//GetEngine()->DrawRenderItems(RenderLayer::Sky/*mCommandList, *//*mRitemLayer[(int)RenderLayer::Sky]*/);

		// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetEngine()->CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList };
	GetEngine()->GetCommandQueue()->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ThrowIfFailed(GetEngine()->GetSwapChain()->Present(0, 0));
	GetEngine()->SetCurrBackBufferIndex();

	// Advance the fence value to mark commands up to this fence point.
	mCurrFrameResource->SetFence(GetEngine()->IncreaseFence());

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	GetEngine()->GetCommandQueue()->Signal(GetEngine()->GetFence(), GetEngine()->GetCurrentFence());
}

