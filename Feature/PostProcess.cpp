#include "PostProcess.h"
#include "GraphicEngine.h"
#include "DeferredShading.h"
#include "Ssao.h"
#include "Ssr.h"

PostProcess::PostProcess(int width, int height, float FarPlane)
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

	//mSsao = new Ssao(mWidth, mHeight);
	mSsr = new Ssr(mWidth, mHeight, FarPlane);

	CreatePostProcessTexture();
	CreatePostProcessView();
}

void PostProcess::CreatePostProcessTexture()
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = GetEngine()->mBackBufferFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	float ClearColor[] = { 0.0f, 0.0f, 1.0f, 0.0f };
	CD3DX12_CLEAR_VALUE optClear(GetEngine()->mBackBufferFormat, ClearColor);
	ThrowIfFailed(GetEngine()->GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&mPostProcessTex)));
}

void PostProcess::CreatePostProcessView()
{
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = GetEngine()->mBackBufferFormat;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;
		GetEngine()->GetDevice()->CreateRenderTargetView(mPostProcessTex.Get(), &rtvDesc,
			GetEngine()->GetDescriptorHeap()->GetRtvDescriptorCpuHandle());

		mPostProcessRtv = GetEngine()->GetDescriptorHeap()->GetRtvDescriptorIndex();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = GetEngine()->mBackBufferFormat;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

		GetEngine()->GetDevice()->CreateShaderResourceView(mPostProcessTex.Get(), &srvDesc,
			GetEngine()->GetDescriptorHeap()->GetSrvDescriptorCpuHandle());
		mPostProcessSrv = GetEngine()->GetDescriptorHeap()->GetSrvDescriptorIndex();
}

void PostProcess::Prepare(ID3D12GraphicsCommandList* cmdList, DeferredShading* deferred)
{
	// Copy 
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetEngine()->CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mPostProcessTex.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
	cmdList->CopyResource(mPostProcessTex.Get(), GetEngine()->CurrentBackBuffer());
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetEngine()->CurrentBackBuffer(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mPostProcessTex.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	SetNormalSrvIndex(deferred->GetGBufferSrv(GBufferType::Normal));
	SetWPosSrvIndex(deferred->GetGBufferSrv(GBufferType::Pos));
	SetFeatureAttrSrvIndex(deferred->GetGBufferSrv(GBufferType::FeatureAttr));
	//SetDeferredSrvIndex(mPostProcessSrv);

}

void PostProcess::BindRootDescriptor(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->SetGraphicsRootDescriptorTable(0, GetEngine()->GetDescriptorHeap()->GetSrvDescriptorGpuHandle(mWPosSrvIndex));
	cmdList->SetGraphicsRootDescriptorTable(1, GetEngine()->GetDescriptorHeap()->GetSrvDescriptorGpuHandle(mNormalSrvIndex));
	cmdList->SetGraphicsRootDescriptorTable(2, GetEngine()->GetDescriptorHeap()->GetSrvDescriptorGpuHandle(mPostProcessSrv));
	cmdList->SetGraphicsRootDescriptorTable(4, GetEngine()->GetDescriptorHeap()->GetSrvDescriptorGpuHandle(mFeatureAttrSrvIndex));
}

void PostProcess::Update(const GameTimer& Timer)
{
mSsr->Update(Timer);
}

void PostProcess::Render(ID3D12GraphicsCommandList* cmdList)
{
	//mSsao->ComputeSsao(cmdList, this);
	mSsr->ComputeSsr(cmdList, this);

// 	//float clearValue[] = { 0.0f, 0.0f, 1.0f, 0.0f };
// 	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetEngine()->CurrentBackBuffer(),
// 		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
// 
// 	// Specify the buffers we are going to render to.
// 	cmdList->OMSetRenderTargets(BUFFER_COUNT, &GetEngine()->CurrentBackBufferView(), true, &GetEngine()->DepthStencilView());
// 
// 	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetEngine()->CurrentBackBuffer(),
// 		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
// 	for (int i = 0; i < BUFFER_COUNT; ++i)
// 	{
// 		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetEngine()->CurrentBackBuffer(),
// 			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
// 	}
// 	cmdList->RSSetViewports(1, &mViewport);
// 	cmdList->RSSetScissorRects(1, &mScissorRect);
// 
// 	// We compute the initial Ssr to AmbientMap0.
// 
// 	// Change to RENDER_TARGET.
// 	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mPostProcessTex.Get(),
// 		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
// 
// 	float clearValue[] = { 1.0f, 1.0f, 1.0f, 1.0f };
// 	CD3DX12_CPU_DESCRIPTOR_HANDLE PostProcessHandle = GetEngine()->GetDescriptorHeap()->GetRtvDescriptorCpuHandle(mPostProcessRtv);
// 	cmdList->ClearRenderTargetView(PostProcessHandle, clearValue, 0, nullptr);
// 
// 	// Specify the buffers we are going to render to.
// 	cmdList->OMSetRenderTargets(1, &PostProcessHandle, true, nullptr);
// 
// 	// Bind the constant buffer for this pass.
// 	cmdList->SetGraphicsRootSignature(mPostProcessRootSignature.Get());
// 
// 	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mPostProcessTex.Get(),
// 		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}