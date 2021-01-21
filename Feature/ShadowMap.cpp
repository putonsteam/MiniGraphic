#include "ShadowMap.h"

void ShadowMap::CreateShadowMapTex()
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
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
	
	// Estimate the scene bounding sphere manually since we know how the scene was constructed.
// The grid is the "widest object" with a width of 20 and depth of 30.0f, and centered at
// the world space origin.  In general, you need to loop over every world space vertex
// position and compute the bounding sphere.
	mSceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
	mSceneBounds.Radius = sqrtf(10.0f*10.0f + 15.0f*15.0f);

	PassCB = make_unique<ConstantBuffer<PassConstants>>(GetEngine()->GetDevice(), 1, true);
	CreateShadowMapTex();
	CreateDescriptors();
	CreatePSO();
}

void ShadowMap::DrawSceneToShadowMap()
{
	ID3D12GraphicsCommandList* mCommandList = GetEngine()->GetCommandList();
	mCommandList->RSSetViewports(1, &mViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Change to DEPTH_WRITE.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	

	// Clear the back buffer and depth buffer.
	mCommandList->ClearDepthStencilView(GetEngine()->GetDescriptorHeap()->GetDsvDescriptorCpuHandle(mShadowMapDsvIndex),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Set null render target because we are only going to draw to
	// depth buffer.  Setting a null render target will disable color writes.
	// Note the active PSO also must specify a render target count of 0.
	mCommandList->OMSetRenderTargets(0, nullptr, false, &GetEngine()->GetDescriptorHeap()->GetDsvDescriptorCpuHandle(mShadowMapDsvIndex));
	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	// Bind the pass constant buffer for the shadow map pass.
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = PassCB->Resource()->GetGPUVirtualAddress() + 0 * passCBByteSize;
	mCommandList->SetGraphicsRootConstantBufferView(1, passCBAddress);
	mCommandList->SetPipelineState(mShadowMapPSO.Get());

	GetEngine()->DrawRenderItems(RenderLayer::Opaque);

	// Change back to GENERIC_READ so we can read the texture in a shader.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap.Get(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));

}

void ShadowMap::CreatePSO()
{
	ShaderState* shader = GetEngine()->GetShader();
	ComPtr<ID3DBlob> vs = shader->CreateVSShader(L"Shader\\Shadows.hlsl");
	ComPtr<ID3DBlob> ps = shader->CreatePSShader(L"Shader\\Shadows.hlsl");
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

void ShadowMap::UpdateShadowTransform()
{
	// Only the first "main" light casts a shadow.
	XMVECTOR lightDir = XMLoadFloat3(&mRotatedLightDirections[0]);
	XMVECTOR lightPos = -2.0f*mSceneBounds.Radius*lightDir;
	XMVECTOR targetPos = XMLoadFloat3(&mSceneBounds.Center);
	XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

	XMStoreFloat3(&mLightPosW, lightPos);

	// Transform bounding sphere to light space.
	XMFLOAT3 sphereCenterLS;
	XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

	// Ortho frustum in light space encloses scene.
	float l = sphereCenterLS.x - mSceneBounds.Radius;
	float b = sphereCenterLS.y - mSceneBounds.Radius;
	float n = sphereCenterLS.z - mSceneBounds.Radius;
	float r = sphereCenterLS.x + mSceneBounds.Radius;
	float t = sphereCenterLS.y + mSceneBounds.Radius;
	float f = sphereCenterLS.z + mSceneBounds.Radius;

	mLightNearZ = n;
	mLightFarZ = f;
	XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	XMMATRIX S = lightView * lightProj*T;
	XMStoreFloat4x4(&mLightView, lightView);
	XMStoreFloat4x4(&mLightProj, lightProj);
	XMStoreFloat4x4(&mShadowTransform, S);
}

void ShadowMap::Update(const GameTimer& Timer)
{
	//mLightRotationAngle += 0.1f*Timer.DeltaTime();

	XMMATRIX R = XMMatrixRotationY(mLightRotationAngle);
	for (int i = 0; i < 3; ++i)
	{
		XMVECTOR lightDir = XMLoadFloat3(&mBaseLightDirections[i]);
		lightDir = XMVector3TransformNormal(lightDir, R);
		XMStoreFloat3(&mRotatedLightDirections[i], lightDir);
	}
	UpdateShadowTransform();
	UpdateShadowPassCB();
}

void ShadowMap::UpdateShadowPassCB()
{
	PassCB->Update();
	XMMATRIX view = XMLoadFloat4x4(&mLightView);
	XMMATRIX proj = XMLoadFloat4x4(&mLightProj);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	//XMStoreFloat4x4(&mShadowPassCB.View, XMMatrixTranspose(view));
	//XMStoreFloat4x4(&mShadowPassCB.InvView, XMMatrixTranspose(invView));
	//XMStoreFloat4x4(&mShadowPassCB.Proj, XMMatrixTranspose(proj));
	//XMStoreFloat4x4(&mShadowPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mShadowPassCB.ViewProj, XMMatrixTranspose(viewProj));
	//XMStoreFloat4x4(&mShadowPassCB.InvViewProj, XMMatrixTranspose(invViewProj));

	mShadowPassCB.EyePosW = mLightPosW;
	//mShadowPassCB.RenderTargetSize = XMFLOAT2((float)mWidth, (float)mHeight);
	//mShadowPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mWidth, 1.0f / mHeight);
	//mShadowPassCB.NearZ = mLightNearZ;
	//mShadowPassCB.FarZ = mLightFarZ;

	PassCB->Update(0, mShadowPassCB);
}






