#include "DescriptorHeap.h"
#include "GraphicEngine.h"

DescriptorHeap::DescriptorHeap(int size)
{
	//
// Create the SRV heap.
//
	HeapSize = size;
	Index = 0;
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = HeapSize;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(GetEngine()->GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

}

int DescriptorHeap::DistributeTexDescriptor(ID3D12Resource* tex)
{
	//
	// Fill out the heap with actual descriptors.
	//
	int HeapIndex = Index;
	Index++;
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	hDescriptor.Offset(HeapIndex, GetEngine()->mCbvSrvUavDescriptorSize);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	srvDesc.Format = tex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = tex->GetDesc().MipLevels;
	GetEngine()->GetDevice()->CreateShaderResourceView(tex, &srvDesc, hDescriptor);
	return HeapIndex;
}

int DescriptorHeap::DistributeCubeDescriptor(ID3D12Resource* tex)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = skyTex->GetDesc().MipLevels;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	srvDesc.Format = skyTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(skyTex.Get(), &srvDesc, hDescriptor);
}

