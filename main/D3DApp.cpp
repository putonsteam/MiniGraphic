#include "D3DApp.h"
#include "MeshInfo.h"
#include "GraphicEngine.h"
#include "LoadTexture.h"
#include "LoadMaterial.h"


bool D3DApp::Init(int Width, int Height, HWND wnd)
{
	GetEngine()->Init(Width, Height, wnd, D3D_FEATURE_LEVEL_11_0);

	GetEngine()->SetPosition(0.0f, 2.0f, -15.0f);

	PassCB = make_unique<ConstantBuffer<PassConstants>>();
	ObjectCB = make_unique<ConstantBuffer<ObjectConstants>>();
	MaterialBuffer = make_unique<ConstantBuffer<MaterialData>>();

	LoadRenderItem();
	GetEngine()->SendCommandAndFulsh();
	mTimer.Reset();
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
	//skullMat->DiffuseSrvHeapIndex = 4;
	//skullMat->NormalSrvHeapIndex = 5;
	skullMat->DiffuseAlbedo = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	skullMat->FresnelR0 = XMFLOAT3(0.6f, 0.6f, 0.6f);
	skullMat->Roughness = 0.2f;
	skullMat->SetDiffuseSrv(L"source/Textures/white1x1.dds");
	//skullMat->SetNormaSrv(L"source/Textures/default_nmap.dds");
	skullRitem->Mat = move(skullMat);

	XMStoreFloat4x4(&skullRitem->World, XMMatrixScaling(0.4f, 0.4f, 0.4f)*XMMatrixTranslation(0.0f, 1.0f, 0.0f));
	skullRitem->TexTransform = MathHelper::Identity4x4();
	skullRitem->ObjCBIndex = 3;
	skullRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//	skullRitem->StartIndexLocation = skull->StartIndexLocation;
// 	skullRitem->BaseVertexLocation = skull->BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::Opaque].push_back(move(skullRitem));

	BuildBaseRootSignature();

	BuildPSO(L"Shader\\Default.hlsl", L"Shader\\Default.hlsl");
}

void D3DApp::UpdateObjectCBs(const GameTimer& gt)
{
	//for (int i = 0; i <= (int)RenderLayer::Count; ++i)
	{
		//for (int j = 0; mRitemLayer[i].size(); ++j)
		{
			RenderItem* e = mRitemLayer[0][0].get();
			// Only update the cbuffer data if the constants have changed.  
			// This needs to be tracked per frame resource.
			//if (e->NumFramesDirty > 0)
			{
				XMMATRIX world = XMLoadFloat4x4(&e->World);
				XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

				ObjectConstants objConstants;
				XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
				XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
				objConstants.MaterialIndex = e->Mat->MatCBIndex;

				ObjectCB->Update(e->ObjCBIndex, objConstants);

				// Next FrameResource need to be updated too.
				e->NumFramesDirty--;
			}
		}
	}
}

void D3DApp::UpdateMaterialBuffer(const GameTimer& gt)
{
	//auto e = *mRitemLayer[(int)RenderLayer::Opaque][1]->Mat;
	//for (auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		LoadMaterial* mat = mRitemLayer[(int)RenderLayer::Opaque][0]->Mat.get();
		//if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialData matData;
			matData.DiffuseAlbedo = mat->DiffuseAlbedo;
			matData.FresnelR0 = mat->FresnelR0;
			matData.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matData.MatTransform, XMMatrixTranspose(matTransform));
			matData.DiffuseMapIndex = mat->DiffuseSrvHeapIndex;
			//matData.NormalMapIndex = mat->NormalSrvHeapIndex;

			MaterialBuffer->Update(mat->MatCBIndex, matData);

			// Next FrameResource need to be updated too.
			//mat->NumFramesDirty--;
		}
	}
}

void D3DApp::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = GetEngine()->mCamera.GetView();
	XMMATRIX proj = GetEngine()->mCamera.GetProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = GetEngine()->mCamera.GetPosition3f();
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)GetEngine()->mClientWidth, (float)GetEngine()->mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / GetEngine()->mClientWidth, 1.0f / GetEngine()->mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { 0.6f, 0.6f, 0.6f };
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mMainPassCB.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };

	PassCB->Update(0, mMainPassCB);
}

void D3DApp::BuildPSO(const wchar_t* vsFile, const wchar_t* psFile)
{
	ShaderState& shader = GetEngine()->GetShader();
	ComPtr<ID3DBlob> vs = shader.CreateVSShader(vsFile);
	ComPtr<ID3DBlob> ps = shader.CreatePSShader(psFile);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { shader.GetLayout().data(), (UINT)shader.GetLayout().size() };
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


void D3DApp::BuildBaseRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable0;
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE texTable1;
	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 1, 0);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[5];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstantBufferView(1);
	slotRootParameter[2].InitAsShaderResourceView(0, 1);
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[4].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);


	auto staticSamplers = GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(5, slotRootParameter,
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
		IID_PPV_ARGS(mBaseRootSignature.GetAddressOf())));
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> D3DApp::GetStaticSamplers()
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

	const CD3DX12_STATIC_SAMPLER_DESC shadow(
		6, // shaderRegister
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,                               // mipLODBias
		16,                                 // maxAnisotropy
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp,
		shadow
	};
}

void D3DApp::CalculateFrameStats()
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.

	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// Compute averages over one second period.
	if ((mTimer.TotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		wstring fpsStr = to_wstring(fps);
		wstring mspfStr = to_wstring(mspf);

		//wstring windowText = mMainWndCaption +
			L"    fps: " + fpsStr +
			L"   mspf: " + mspfStr;

		//SetWindowText(mhMainWnd, windowText.c_str());

		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}


void D3DApp::Run()
{
	mTimer.Tick();
	CalculateFrameStats();
 	Update(mTimer);
 	Draw(mTimer);
}

void D3DApp::Update(const GameTimer& gt)
{
	GetEngine()->OnKeyboardInput(gt);

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

	mLightRotationAngle += 0.1f*gt.DeltaTime();

	XMMATRIX R = XMMatrixRotationY(mLightRotationAngle);
	for (int i = 0; i < 3; ++i)
	{
		XMVECTOR lightDir = XMLoadFloat3(&mBaseLightDirections[i]);
		lightDir = XMVector3TransformNormal(lightDir, R);
		XMStoreFloat3(&mRotatedLightDirections[i], lightDir);
	}

	//AnimateMaterials(gt);
	UpdateObjectCBs(gt);
	UpdateMaterialBuffer(gt);
	//UpdateShadowTransform(gt);
	UpdateMainPassCB(gt);
	//UpdateShadowPassCB(gt);
	//UpdateSsaoCB(gt);
}

void D3DApp::Draw(const GameTimer& gt)
{
	ComPtr<ID3D12CommandAllocator> cmdListAlloc = mCurrFrameResource->GetCurrentCommandAllocator();
	auto mCommandList = GetEngine()->mCommandList;

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mBasePSO.Get()));

	mCommandList->RSSetViewports(1, &GetEngine()->mScreenViewport);
	mCommandList->RSSetScissorRects(1, &GetEngine()->mScissorRect);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetEngine()->CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(GetEngine()->CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(GetEngine()->DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &GetEngine()->CurrentBackBufferView(), true, &GetEngine()->DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { GetEngine()->GetDescriptorHeap()->mSrvDescriptorHeap.Get() };
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
	//mCommandList->SetGraphicsRootDescriptorTable(3, mNullSrv);

	// Bind all the textures used in this scene.  Observe
	// that we only have to specify the first descriptor in the table.  
	// The root signature knows how many descriptors are expected in the table.
	mCommandList->SetGraphicsRootDescriptorTable(4, GetEngine()->GetDescriptorHeap()->mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

	// 	mCommandList->SetPipelineState(mPSOs["sky"].Get());
	// 	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Sky]);

		// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetEngine()->CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	GetEngine()->mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ThrowIfFailed(GetEngine()->mSwapChain->Present(0, 0));
	GetEngine()->mCurrBackBuffer = (GetEngine()->mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Advance the fence value to mark commands up to this fence point.
	mCurrFrameResource->SetFence(GetEngine()->IncreaseFence());

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	GetEngine()->mCommandQueue->Signal(GetEngine()->GetFence(), GetEngine()->GetCurrentFence());
}

void D3DApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<unique_ptr<RenderItem>>& ritems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));


	// For each render item...
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i].get();

		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = ObjectCB->Resource()->GetGPUVirtualAddress() + ri->ObjCBIndex*objCBByteSize;

		cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}


