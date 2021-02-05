#include "Ssao.h"
#include <DirectXPackedVector.h>

using namespace DirectX::PackedVector;

Ssao::Ssao(UINT width, UINT height)
{
	mWidth = width;
	mHeight = height;

	mViewport.TopLeftX = 0.0f;
	mViewport.TopLeftY = 0.0f;
	mViewport.Width = mWidth / 2.0f;
	mViewport.Height = mHeight / 2.0f;
	mViewport.MinDepth = 0.0f;
	mViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, (int)mWidth / 2, (int)mHeight / 2 };


	mCBSsao = make_unique<ConstantBuffer<CBSsao>>(GetEngine()->GetDevice(), 1, true);
	CreateDepthDescriptors();

	CreateSsaoTex();
	CreateSsaoDescriptors();

	CreateRandomVectorTexture();
	CreateRandomDescriptors();

	BuildSsaoRootSignature();
	CreateSsaoPSO();

	InitSsaoCb();
}

void Ssao::Update(const GameTimer& Timer)
{
	UpdateSsaoCB(Timer);
}

void Ssao::CreateDepthDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	GetEngine()->GetDevice()->CreateShaderResourceView(GetEngine()->GetDsBuffer(), &srvDesc, GetEngine()->GetDescriptorHeap()->GetSrvDescriptorCpuHandle());
	mDepthSrvIndex = GetEngine()->GetDescriptorHeap()->GetSrvDescriptorIndex();
}

void Ssao::CreateSsaoPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoPsoDesc;

	ZeroMemory(&ssaoPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	ShaderState* shader = GetEngine()->GetShader();
	ComPtr<ID3DBlob> vs = shader->CreateVSShader(L"Shader\\Ssao.hlsl");
	ComPtr<ID3DBlob> ps = shader->CreatePSShader(L"Shader\\Ssao.hlsl");
	ssaoPsoDesc.InputLayout = { nullptr, 0 };
	ssaoPsoDesc.pRootSignature = mSsaoRootSignature.Get();
	ssaoPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(vs->GetBufferPointer()),
		vs->GetBufferSize()
	};
	ssaoPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(ps->GetBufferPointer()),
		ps->GetBufferSize()
	};

	ssaoPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	ssaoPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	ssaoPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	ssaoPsoDesc.SampleMask = UINT_MAX;
	ssaoPsoDesc.NumRenderTargets = 1;
	ssaoPsoDesc.DepthStencilState.DepthEnable = false;
	ssaoPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	ssaoPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	ssaoPsoDesc.RTVFormats[0] = AmbientMapFormat;
	ssaoPsoDesc.SampleDesc.Count = 1;
	ssaoPsoDesc.SampleDesc.Quality = 0;
	ssaoPsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	ThrowIfFailed(GetEngine()->GetDevice()->CreateGraphicsPipelineState(&ssaoPsoDesc, IID_PPV_ARGS(&mSsaoPSO)));
}

void Ssao::BuildSsaoRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable0;
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE texTable1;
	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE texTable2;
	texTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);

	CD3DX12_DESCRIPTOR_RANGE texTable3;
	texTable3.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 0);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[5];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[2].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable2, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[4].InitAsDescriptorTable(1, &texTable3, D3D12_SHADER_VISIBILITY_PIXEL);

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
		IID_PPV_ARGS(mSsaoRootSignature.GetAddressOf())));
}

void Ssao::ComputeSsao(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->RSSetViewports(1, &mViewport);
	cmdList->RSSetScissorRects(1, &mScissorRect);

	// We compute the initial SSAO to AmbientMap0.

	// Change to RENDER_TARGET.
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mSsaoMap.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	float clearValue[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	CD3DX12_CPU_DESCRIPTOR_HANDLE SsaoHandle = GetEngine()->GetDescriptorHeap()->GetRtvDescriptorCpuHandle(mSsaoRtvIndex);
	cmdList->ClearRenderTargetView(SsaoHandle, clearValue, 0, nullptr);

	// Specify the buffers we are going to render to.
	cmdList->OMSetRenderTargets(1, &SsaoHandle, true, nullptr);

	// Bind the constant buffer for this pass.
	cmdList->SetGraphicsRootSignature(mSsaoRootSignature.Get());
	auto ssaoCBAddress = mCBSsao->Resource()->GetGPUVirtualAddress();
	cmdList->SetGraphicsRootConstantBufferView(0, ssaoCBAddress);

	cmdList->SetGraphicsRootDescriptorTable(1, GetEngine()->GetDescriptorHeap()->GetSrvDescriptorGpuHandle(mWPosSrvIndex));
	cmdList->SetGraphicsRootDescriptorTable(2, GetEngine()->GetDescriptorHeap()->GetSrvDescriptorGpuHandle(mNormalSrvIndex));
	cmdList->SetGraphicsRootDescriptorTable(3, GetEngine()->GetDescriptorHeap()->GetSrvDescriptorGpuHandle(mDepthSrvIndex));
	cmdList->SetGraphicsRootDescriptorTable(4, GetEngine()->GetDescriptorHeap()->GetSrvDescriptorGpuHandle(mRandomVectorSrvIndex));

	cmdList->SetPipelineState(mSsaoPSO.Get());

	// Draw fullscreen quad.
	cmdList->IASetVertexBuffers(0, 0, nullptr);
	cmdList->IASetIndexBuffer(nullptr);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(6, 1, 0, 0);

	// Change back to GENERIC_READ so we can read the texture in a shader.
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mSsaoMap.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));

}

void Ssao::CreateSsaoTex()
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	// Ambient occlusion maps are at half resolution.
	texDesc.Width = mWidth / 2;
	texDesc.Height = mHeight / 2;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	texDesc.Format = AmbientMapFormat;

	float ambientClearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	D3D12_CLEAR_VALUE optClear = CD3DX12_CLEAR_VALUE(AmbientMapFormat, ambientClearColor);

	ThrowIfFailed(GetEngine()->GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&mSsaoMap)));
}

void Ssao::CreateSsaoDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	srvDesc.Format = AmbientMapFormat;
	GetEngine()->GetDevice()->CreateShaderResourceView(mSsaoMap.Get(), &srvDesc, GetEngine()->GetDescriptorHeap()->GetSrvDescriptorCpuHandle());
	mSsaoSrvIndex = GetEngine()->GetDescriptorHeap()->GetSrvDescriptorIndex();

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;
	rtvDesc.Format = AmbientMapFormat;

	GetEngine()->GetDevice()->CreateRenderTargetView(mSsaoMap.Get(), &rtvDesc, GetEngine()->GetDescriptorHeap()->GetRtvDescriptorCpuHandle());
	mSsaoRtvIndex = GetEngine()->GetDescriptorHeap()->GetRtvDescriptorIndex();
}

void Ssao::CreateRandomVectorTexture()
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = 256;
	texDesc.Height = 256;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	PackedVector::XMCOLOR initData[256 * 256];
	for (int i = 0; i < 256; ++i)
	{
		for (int j = 0; j < 256; ++j)
		{
			// Random vector in [0,1].  We will decompress in shader to [-1,1].
			XMFLOAT3 v(MathHelper::RandF(), MathHelper::RandF(), MathHelper::RandF());
			initData[i * 256 + j] = PackedVector::XMCOLOR(v.x, v.y, v.z, 0.0f);
		}
	}	
			// 			char dbg_out[50];
			// 			sprintf(dbg_out, "test: %f %f %f\n", v.x, v.y, v.z);
			// 			//wprintf_s(, );
			// 			std::wstring wc(50, L'#');
			// 			//wchar_t wc[50];
			// 			mbstowcs(&wc[0], dbg_out, 50);
			// 			OutputDebugString(wc.c_str());
	mRandomVectorMap = GetEngine()->CreateArray2DBuffer(texDesc, (void*)initData, sizeof(PackedVector::XMCOLOR), mRandomVectorMapUploadBuffer);
}

void Ssao::CreateRandomDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	GetEngine()->GetDevice()->CreateShaderResourceView(mRandomVectorMap.Get(), &srvDesc, GetEngine()->GetDescriptorHeap()->GetSrvDescriptorCpuHandle());
	mRandomVectorSrvIndex = GetEngine()->GetDescriptorHeap()->GetSrvDescriptorIndex();
}

void Ssao::InitSsaoCb()
{
	BuildOffsetVectors();

	// Coordinates given in view space.
	ssaoCB.OcclusionRadius = 0.5f;
	ssaoCB.OcclusionFadeStart = 0.2f;
	ssaoCB.OcclusionFadeEnd = 1.0f;
	ssaoCB.SurfaceEpsilon = 0.05f;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE Ssao::GetSsaoSrvGpuHandle()
{
	return GetEngine()->GetDescriptorHeap()->GetSrvDescriptorGpuHandle(mSsaoSrvIndex); 
}

void Ssao::BuildOffsetVectors()
{
	// Start with 14 uniformly distributed vectors.  We choose the 8 corners of the cube
	// and the 6 center points along each cube face.  We always alternate the points on 
	// opposites sides of the cubes.  This way we still get the vectors spread out even
	// if we choose to use less than 14 samples.

	// 8 cube corners
	ssaoCB.OffsetVectors[0] = XMFLOAT4(+1.0f, +1.0f, +1.0f, 0.0f);
	ssaoCB.OffsetVectors[1] = XMFLOAT4(-1.0f, -1.0f, -1.0f, 0.0f);

	ssaoCB.OffsetVectors[2] = XMFLOAT4(-1.0f, +1.0f, +1.0f, 0.0f);
	ssaoCB.OffsetVectors[3] = XMFLOAT4(+1.0f, -1.0f, -1.0f, 0.0f);

	ssaoCB.OffsetVectors[4] = XMFLOAT4(+1.0f, +1.0f, -1.0f, 0.0f);
	ssaoCB.OffsetVectors[5] = XMFLOAT4(-1.0f, -1.0f, +1.0f, 0.0f);

	ssaoCB.OffsetVectors[6] = XMFLOAT4(-1.0f, +1.0f, -1.0f, 0.0f);
	ssaoCB.OffsetVectors[7] = XMFLOAT4(+1.0f, -1.0f, +1.0f, 0.0f);

	// 6 centers of cube faces
	ssaoCB.OffsetVectors[8] = XMFLOAT4(-1.0f, 0.0f, 0.0f, 0.0f);
	ssaoCB.OffsetVectors[9] = XMFLOAT4(+1.0f, 0.0f, 0.0f, 0.0f);

	ssaoCB.OffsetVectors[10] = XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
	ssaoCB.OffsetVectors[11] = XMFLOAT4(0.0f, +1.0f, 0.0f, 0.0f);

	ssaoCB.OffsetVectors[12] = XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);
	ssaoCB.OffsetVectors[13] = XMFLOAT4(0.0f, 0.0f, +1.0f, 0.0f);

	for (int i = 0; i < 14; ++i)
	{
		// Create random lengths in [0.25, 1.0].
		float s = MathHelper::RandF(0.25f, 1.0f);

		XMVECTOR v = s * XMVector4Normalize(XMLoadFloat4(&ssaoCB.OffsetVectors[i]));

		XMStoreFloat4(&ssaoCB.OffsetVectors[i], v);
	}
}

void Ssao::UpdateSsaoCB(const GameTimer& Timer)
{
	mCBSsao->Update();

	XMMATRIX view = GetEngine()->GetView();
	XMMATRIX proj = GetEngine()->GetProj();

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	XMStoreFloat4x4(&ssaoCB.gView, XMMatrixTranspose(view));
	XMStoreFloat4x4(&ssaoCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&ssaoCB.InvProj, XMMatrixTranspose(XMMatrixInverse(&XMMatrixDeterminant(proj), proj)));
	XMStoreFloat4x4(&ssaoCB.ProjTex, XMMatrixTranspose(proj*T));

	mCBSsao->Update(0, ssaoCB);
}


