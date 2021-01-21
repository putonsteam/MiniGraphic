#include "GraphicEngine.h"

IMPLEMENT_SINGLE(GraphicEngine)

bool GraphicEngine::Init(int Width, int Height, HWND wnd, D3D_FEATURE_LEVEL level)
{
	mTimer.Reset();
	mClientWidth = Width;
	mClientHeight = Height;
	mhMainWnd = wnd;
	m_D3DMinFeatureLevel = level;
	InitDevice();
	InitGPUCommand();
	InitDescriptorHeap(10);
	InitDesHeap();
	InitSwapchainAndRvt();
	InitDsv();
	InitViewportAndScissor();
	mFrameResource = new FrameResource();
	mFrameResource->Init();
	return true;
}

bool GraphicEngine::InitDevice()
{
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_DxgiFactory)));

	ComPtr<IDXGIAdapter1> adapter;
	ComPtr<IDXGIFactory6> factory6;
	HRESULT hr = m_DxgiFactory.As(&factory6);
	if (FAILED(hr))
	{
		//throw exception("DXGI 1.6 not supported");
		return false;
	}
	// if DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE is specified, then the highest-performing adapter is at index 0, the second-highest is at index 1, and so on.
	for (UINT adapterID = 0; DXGI_ERROR_NOT_FOUND != factory6->EnumAdapterByGpuPreference(adapterID, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)); ++adapterID)
	{
		DXGI_ADAPTER_DESC1 desc;
		ThrowIfFailed(adapter->GetDesc1(&desc));
		m_AdapterDescription = desc.Description;
		m_Adapter = adapter.Detach();
		break;
	}

	// Create the DX12 API m_D3DDevice object.
	ThrowIfFailed(D3D12CreateDevice(m_Adapter.Get(), m_D3DMinFeatureLevel, IID_PPV_ARGS(&m_D3DDevice)));

	return true;
}

void GraphicEngine::SetPosition(float x, float y, float z)
{
	mCamera.SetPosition(x, y, z);
}

void GraphicEngine::SetPosition(const XMFLOAT3& v)
{
	mCamera.SetPosition(v);

}

XMVECTOR GraphicEngine::GetPosition()const
{
	return mCamera.GetPosition();
}

XMFLOAT3 GraphicEngine::GetPosition3f()const
{
	return mCamera.GetPosition3f();
}

XMMATRIX GraphicEngine::GetView()const
{
	return mCamera.GetView();
}

XMMATRIX GraphicEngine::GetProj()const
{
	return mCamera.GetProj();
}

void GraphicEngine::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void GraphicEngine::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		mCamera.Pitch(dy);
		mCamera.RotateY(dx);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;

}

GraphicEngine::GraphicEngine()
{

}

void GraphicEngine::OnKeyboardInput()
{
	const float dt = mTimer.DeltaTime();

	if (GetAsyncKeyState('W') & 0x8000)
		mCamera.Walk(10.0f*dt);

	if (GetAsyncKeyState('S') & 0x8000)
		mCamera.Walk(-10.0f*dt);

	if (GetAsyncKeyState('A') & 0x8000)
		mCamera.Strafe(-10.0f*dt);

	if (GetAsyncKeyState('D') & 0x8000)
		mCamera.Strafe(10.0f*dt);

	mCamera.UpdateViewMatrix();
}

void GraphicEngine::SendCommandAndFulsh()
{
	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	Flush();
}

void GraphicEngine::Flush()
{
	// Advance the fence value to mark commands up to this fence point.
	m_CurrentFence++;

	// Add an instruction to the command queue to set a new fence point.  Because we 
	// are on the GPU timeline, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
	ThrowIfFailed(mCommandQueue->Signal(m_Fence.Get(), m_CurrentFence));

	// Wait until the GPU has completed commands up to this fence point.
	if (m_Fence->GetCompletedValue() < m_CurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

		// Fire event when GPU hits current fence.  
		ThrowIfFailed(m_Fence->SetEventOnCompletion(m_CurrentFence, eventHandle));

		// Wait until the GPU hits current fence event is fired.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

ID3D12Resource* GraphicEngine::CurrentBackBuffer()const
{
	return mSwapChainBuffer[mCurrBackBufferIndex].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE GraphicEngine::CurrentBackBufferView()const
{
	return mDescriptorHeap->GetRtvDescriptorCpuHandle(mCurrBackBufferIndex);
}

D3D12_CPU_DESCRIPTOR_HANDLE GraphicEngine::DepthStencilView()const
{
	return mDescriptorHeap->GetDsvDescriptorCpuHandle(0);
}

void GraphicEngine::InitDescriptorHeap(int size)
{
	mDescriptorHeap = new DescriptorHeap();
	mDescriptorHeap->CreateSrvDescriptorHeap(size);
	mDescriptorHeap->CreateRtvDescriptorHeap(SwapChainBufferCount);
	mDescriptorHeap->CreateDsvDescriptorHeap(2);
}

void GraphicEngine::InitGPUCommand()
{
	ThrowIfFailed(m_D3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&m_Fence)));

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(m_D3DDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	ThrowIfFailed(m_D3DDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));

	ThrowIfFailed(m_D3DDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mDirectCmdListAlloc.Get(), // Associated command allocator
		nullptr,                   // Initial PipelineStateObject
		IID_PPV_ARGS(mCommandList.GetAddressOf())));

	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	//mCommandList->Close();
}

void GraphicEngine::InitDesHeap()
{
	mRtvDescriptorSize = m_D3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = m_D3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = m_D3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void GraphicEngine::InitSwapchainAndRvt()
{
	mSwapChain.Reset();

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = mClientWidth;
	sd.BufferDesc.Height = mClientHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = mBackBufferFormat;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = SwapChainBufferCount;
	sd.OutputWindow = mhMainWnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Note: Swap chain uses queue to perform flush.
	ThrowIfFailed(m_DxgiFactory->CreateSwapChain(
		mCommandQueue.Get(),
		&sd,
		mSwapChain.GetAddressOf()));


	mCurrBackBufferIndex = 0;

	for (UINT i = 0; i < SwapChainBufferCount; i++)
	{
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
		m_D3DDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, mDescriptorHeap->GetRtvDescriptorCpuHandle());
	}
}

void GraphicEngine::InitDsv()
{
	// Create the depth/stencil buffer and view.
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = mClientWidth;
	depthStencilDesc.Height = mClientHeight;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = mDepthStencilFormat;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	ThrowIfFailed(m_D3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

	// Create descriptor to mip level 0 of entire resource using the format of the resource.
	m_D3DDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, mDescriptorHeap->GetDsvDescriptorCpuHandle());

	//ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Transition the resource from its initial state to be used as a depth buffer.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// Wait until resize is complete.
	Flush();
}

void GraphicEngine::InitViewportAndScissor()
{
	// Update the viewport transform to cover the client area.
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(mClientWidth);
	mScreenViewport.Height = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, mClientWidth, mClientHeight };
}

void GraphicEngine::Run()
{
	mTimer.Tick();
	CalculateFrameStats();
	Update(mTimer);
}

void GraphicEngine::Update(const GameTimer& Timer)
{
	OnKeyboardInput();
	UpdateShaderParameter(Timer);
}

void GraphicEngine::UpdateShaderParameter(const GameTimer& Timer)
{
	UpdateObjectCBs(Timer);
	UpdateMaterialBuffer(Timer);
	UpdateMainPassCB(Timer);
}

void GraphicEngine::UpdateObjectCBs(const GameTimer& Timer)
{
	ObjectCB->Update();
	for (int i = 0; i != (int)RenderLayer::Count; ++i)
	{
		for (int j = 0; j != mRitemLayer[i].size(); ++j)
		{
			RenderItem* e = mRitemLayer[i][j].get();
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

void GraphicEngine::UpdateMaterialBuffer(const GameTimer& Timer)
{
	MaterialBuffer->Update();
	for (int i = 0; i != (int)RenderLayer::Count; ++i)
	{
		for (int j = 0; j != mRitemLayer[i].size(); ++j)
		{
			LoadMaterial* mat = (mRitemLayer[i][j]->Mat).get();
			//for (auto& e : mMaterials)
			//{
				// Only update the cbuffer data if the constants have changed.  If the cbuffer
				// data changes, it needs to be updated for each FrameResource.
				//LoadMaterial* mat = mRitemLayer[(int)RenderLayer::Opaque][0]->Mat.get();
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
}

void GraphicEngine::UpdateMainPassCB(const GameTimer& Timer)
{
	PassCB->Update();
	XMMATRIX view = /*GetEngine()->*/GetCamera()->GetView();
	XMMATRIX proj = /*GetEngine()->*/GetCamera()->GetProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	mMainPassCB.EyePosW = /*GetEngine()->*/GetCamera()->GetPosition3f();
	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { 0.6f, 0.6f, 0.6f };
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mMainPassCB.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };

	PassCB->Update(0, mMainPassCB);
}

void GraphicEngine::CreateShaderParameter()
{
	int count = 0;
	for (int i = 0; i != (int)RenderLayer::Count; ++i)
	{
		for (int j = 0; j != mRitemLayer[i].size(); ++j)
		{
			count++;
		}
	}
	PassCB = make_unique<ConstantBuffer<PassConstants>>(m_D3DDevice.Get(), 1, true);
	ObjectCB = make_unique<ConstantBuffer<ObjectConstants>>(m_D3DDevice.Get(), count, true);
	MaterialBuffer = make_unique<ConstantBuffer<MaterialData>>(m_D3DDevice.Get(), count, false);
}

void GraphicEngine::SetBaseRootSignature0()
{

}

void GraphicEngine::SetBaseRootSignature1()
{
	mCommandList->SetGraphicsRootConstantBufferView(1, PassCB->Resource()->GetGPUVirtualAddress());
}

void GraphicEngine::SetBaseRootSignature2()
{
	mCommandList->SetGraphicsRootShaderResourceView(2, MaterialBuffer->Resource()->GetGPUVirtualAddress());
}

void GraphicEngine::BuildBaseRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable0;
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE texTable1;
	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 2, 0);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[6];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstantBufferView(1);
	slotRootParameter[2].InitAsShaderResourceView(0, 1);
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[4].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[5].InitAsConstantBufferView(2);


	auto staticSamplers = GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(6, slotRootParameter,
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


std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GraphicEngine::GetStaticSamplers()
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

void GraphicEngine::AddRenderItem(RenderLayer layer, unique_ptr<RenderItem>& item)
{
	mRitemLayer[(int)layer].push_back(move(item));
}

void GraphicEngine::DrawRenderItems(RenderLayer layer/*ID3D12GraphicsCommandList* cmdList, *//*const std::vector<unique_ptr<RenderItem>>& ritems*/)
{
	const vector<unique_ptr<RenderItem>>& ritems = mRitemLayer[(int)layer];
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));


	// For each render item...
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i].get();

		mCommandList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		mCommandList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		mCommandList->IASetPrimitiveTopology(ri->PrimitiveType);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = ObjectCB->Resource()->GetGPUVirtualAddress() + ri->ObjCBIndex*objCBByteSize;

		mCommandList->SetGraphicsRootConstantBufferView(0, objCBAddress);

		mCommandList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

void GraphicEngine::CalculateFrameStats()
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

ComPtr<ID3D12Resource> GraphicEngine::CreateDefaultBuffer(
	const void* initData,
	UINT64 byteSize,
	ComPtr<ID3D12Resource>& uploadBuffer)
{
	ComPtr<ID3D12Resource> defaultBuffer;

	// Create the actual default buffer resource.
	// this is placed in GPU memeory, but cannot write to the vertex buffer in the default heap, so need uploadBuffer
	ThrowIfFailed(m_D3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

	// In order to copy CPU memory data into our default buffer, we need to create
	// an intermediate upload heap. 
	ThrowIfFailed(m_D3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploadBuffer.GetAddressOf())));


	// Describe the data we want to copy into the default buffer.
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	// Schedule to copy the data to the default buffer resource.  At a high level, the helper function UpdateSubresources
	// will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
	// the intermediate upload heap data will be copied to mBuffer.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources<1>(mCommandList.Get(), defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	// Note: uploadBuffer has to be kept alive after the above function calls because
	// the command list has not been executed yet that performs the actual copy.
	// The caller can Release the uploadBuffer after it knows the copy has been executed.


	return defaultBuffer;
}

ComPtr<ID3D12Resource> GraphicEngine::CreateArray2DBuffer(
	D3D12_RESOURCE_DESC& texDesc,
	const void* initData,
	UINT64 byteSize,
	ComPtr<ID3D12Resource>& uploadBuffer)
{
	ComPtr<ID3D12Resource> defaultBuffer;

ThrowIfFailed(m_D3DDevice->CreateCommittedResource(
	&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
	D3D12_HEAP_FLAG_NONE,
	&texDesc,
	D3D12_RESOURCE_STATE_GENERIC_READ,
	nullptr,
	IID_PPV_ARGS(&defaultBuffer)));

//
// In order to copy CPU memory data into our default buffer, we need to create
// an intermediate upload heap. 
//

const UINT num2DSubresources = texDesc.DepthOrArraySize * texDesc.MipLevels;
const UINT64 uploadBufferSize = GetRequiredIntermediateSize(defaultBuffer.Get(), 0, num2DSubresources);

ThrowIfFailed(m_D3DDevice->CreateCommittedResource(
	&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
	D3D12_HEAP_FLAG_NONE,
	&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
	D3D12_RESOURCE_STATE_GENERIC_READ,
	nullptr,
	IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

D3D12_SUBRESOURCE_DATA subResourceData = {};
subResourceData.pData = initData;
subResourceData.RowPitch = texDesc.Height * byteSize;
subResourceData.SlicePitch = subResourceData.RowPitch * texDesc.Width;

//
// Schedule to copy the data to the default resource, and change states.
// Note that mCurrSol is put in the GENERIC_READ state so it can be 
// read by a shader.
//

mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
	D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));
UpdateSubresources(mCommandList.Get(), defaultBuffer.Get(), uploadBuffer.Get(),
	0, 0, num2DSubresources, &subResourceData);
mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
	D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
}

GraphicEngine* GetEngine()
{
	return CALL_SINGLE(GraphicEngine);
}