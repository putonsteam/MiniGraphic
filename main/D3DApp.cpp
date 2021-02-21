#include "D3DApp.h"
#include "MeshInfo.h"
#include "GraphicEngine.h"
#include "LoadTexture.h"
#include "LoadMaterial.h"
#include "PostProcess.h"
#include "Ssao.h"

D3DApp::D3DApp()
{
	mCBFeature = nullptr;
}

bool D3DApp::Init(int Width, int Height, HWND wnd)
{
	GetEngine()->Init(Width, Height, wnd, D3D_FEATURE_LEVEL_11_0);
	ThrowIfFailed(GetEngine()->GetCommandList()->Reset(GetEngine()->GetCommandAlloc(), nullptr));

	mNearPlane = 1.0f;
	mFarPlane = 1000.0f;
	GetEngine()->SetPosition(0.0f, 2.0f, -15.0f);
	GetEngine()->SetLens(0.25f*MathHelper::Pi, GetEngine()->AspectRatio(), mNearPlane, mFarPlane);

	LoadRenderItem();
	mCBFeature = make_unique<ConstantBuffer<CBFeature>>(GetEngine()->GetDevice(), 1, true);

	mShadowMap = new ShadowMap(2048, 2048);
	mSsao = new Ssao(Width, Height);
	m_DeferredShading = new DeferredShading(Width, Height);
	m_PostProcess = new PostProcess(Width, Height, mFarPlane);

	GetEngine()->SendCommandAndFulsh();
	return true;
}

void D3DApp::LoadRenderItem()
{
	GetEngine()->BuildBaseRootSignature();

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
	skullMat->SetDiffuseSrv(L"source/Textures/white1x1.dds");
	skullRitem->Mat = move(skullMat);

	XMStoreFloat4x4(&skullRitem->World, XMMatrixScaling(0.4f, 0.4f, 0.4f)*XMMatrixTranslation(0.0f, 1.0f, 0.0f));
	skullRitem->TexTransform = MathHelper::Identity4x4();
	skullRitem->ObjCBIndex = 0;
	skullRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	GetEngine()->AddRenderItem(RenderLayer::Opaque, skullRitem);

	auto grid = std::make_unique<MeshInfo>();
	grid->CreateGrid(20.0f, 30.0f, 60, 40);
	auto gridRitem = std::make_unique<RenderItem>();
	gridRitem->IndexCount = grid->IndexCount;

	auto tile0 = std::make_unique<LoadMaterial>();
	tile0->Name = "tile0";
	tile0->MatCBIndex = 1;
	tile0->SetDiffuseSrv(L"source/Textures/tile.dds");
	tile0->DiffuseAlbedo = XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f);
	tile0->FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);

	tile0->Roughness = 0.1f;

	gridRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(8.0f, 8.0f, 1.0f));
	gridRitem->ObjCBIndex = 1;
	gridRitem->Mat = move(tile0);
	gridRitem->Geo = move(grid);
	gridRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	GetEngine()->AddRenderItem(RenderLayer::Opaque, gridRitem);

	mSky.LoadRenderItem();
	GetEngine()->CreateShaderParameter();

}

void D3DApp::Run()
{
	GetEngine()->Run();
 	Update(GetEngine()->GetTimer());
	m_PostProcess->Update(GetEngine()->GetTimer());
 	Render(GetEngine()->GetTimer());
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

	mShadowMap->Update(Timer);
	mSsao->Update(Timer);
	UpdateFeatureCB(Timer);

}

void D3DApp::UpdateFeatureCB(const GameTimer& Timer)
{
	mCBFeature->Update();
	XMMATRIX shadowTransform = XMLoadFloat4x4(&(mShadowMap->GetShadowTransform()));
	XMStoreFloat4x4(&mFeatureCB.ShadowTransform, XMMatrixTranspose(shadowTransform));
	mCBFeature->Update(0, mFeatureCB);
}

void D3DApp::Render(const GameTimer& Timer)
{
	ComPtr<ID3D12CommandAllocator> cmdListAlloc = mCurrFrameResource->GetCurrentCommandAllocator();
	auto mCommandList = GetEngine()->GetCommandList();

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), nullptr));// mBasePSO.Get()));

	ID3D12DescriptorHeap* descriptorHeaps[] = { GetEngine()->GetSrvDescHeap() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(GetEngine()->GetBaseRootSignature());

	// Bind the sky cube map.  For our demos, we just use one "world" cube map representing the environment
	// from far away, so all objects will use the same cube map and we only need to set it once per-frame.  
	// If we wanted to use "local" cube maps, we would have to change them per-object, or dynamically
	// index into an array of cube maps.
	GetEngine()->SetBaseRootSignature1();
	GetEngine()->SetBaseRootSignature3();
	mCommandList->SetGraphicsRootDescriptorTable(5, GetEngine()->GetSrvDescHeap()->GetGPUDescriptorHandleForHeapStart());
	m_DeferredShading->RenderGBuffer(mCommandList);

	mShadowMap->DrawSceneToShadowMap();

	mSsao->SetNormalSrvIndex(m_DeferredShading->GetGBufferSrv(GBufferType::Normal));
	mSsao->SetWPosSrvIndex(m_DeferredShading->GetGBufferSrv(GBufferType::Pos));
	mSsao->ComputeSsao(mCommandList,m_PostProcess);

	mCommandList->SetGraphicsRootSignature(GetEngine()->GetBaseRootSignature());
	GetEngine()->SetBaseRootSignature1();
	mCommandList->SetGraphicsRootConstantBufferView(2, mCBFeature->Resource()->GetGPUVirtualAddress());
	GetEngine()->SetBaseRootSignature3();
	mCommandList->SetGraphicsRootDescriptorTable(4, mSky.GetSkyHeapStart());
	mCommandList->SetGraphicsRootDescriptorTable(5, GetEngine()->GetSrvDescHeap()->GetGPUDescriptorHandleForHeapStart());
	mCommandList->SetGraphicsRootDescriptorTable(6, mSsao->GetSsaoSrvGpuHandle());
	mCommandList->SetGraphicsRootDescriptorTable(7, m_DeferredShading->GetGBufferSrvGpuHandle());

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetEngine()->CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	m_DeferredShading->Render(mCommandList);


	mSky.Draw(Timer);

	m_PostProcess->Prepare(mCommandList, m_DeferredShading);
	m_PostProcess->Render(mCommandList);
 	//mSsao->SetNormalSrvIndex(m_DeferredShading->GetGBufferSrv(GBufferType::Normal));
 	//mSsao->SetWPosSrvIndex(m_DeferredShading->GetGBufferSrv(GBufferType::Pos));

	//m_PostProcess->Render(mCommandList);

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


