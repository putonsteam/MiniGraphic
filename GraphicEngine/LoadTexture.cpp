#include "LoadTexture.h"
#include "DDSTextureLoader.h"
#include "GraphicEngine.h"

int LoadTexture::Load(const wchar_t* file)
{
	Texture texMap;
	texMap.DescriptorIndex++;
	texMap.Filename = file;
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(GetEngine()->GetDevice().Get(),
			GetEngine()->GetCommandList().Get(), file,
			texMap.Resource, texMap.UploadHeap));
		TextureList.push_back(texMap);
		return texMap.DescriptorIndex;
}


void LoadTexture::BuildTextureDescriptorHeaps()
{
	//
	// Create the SRV heap.
	//
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = TextureList.size();
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(GetEngine()->GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	//
	// Fill out the heap with actual descriptors.
	//
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());


	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	for (UINT i = 0; i < (UINT)TextureList.size(); ++i)
	{
		srvDesc.Format = TextureList[i].Resource->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = TextureList[i].Resource->GetDesc().MipLevels;
		GetEngine()->GetDevice()->CreateShaderResourceView(TextureList[i].Resource.Get(), &srvDesc, hDescriptor);

		// next descriptor
		hDescriptor.Offset(1, GetEngine()->mCbvSrvUavDescriptorSize);
	}
}
