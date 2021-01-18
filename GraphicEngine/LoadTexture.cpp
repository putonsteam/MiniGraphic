#include "LoadTexture.h"
#include "DDSTextureLoader.h"
#include "GraphicEngine.h"

int LoadTexture::Load(const wchar_t* file)
{
	Texture texMap;
	texMap.Filename = file;
	ThrowIfFailed(CreateDDSTextureFromFile12(GetEngine()->GetDevice(),
		GetEngine()->GetCommandList(), file,
		texMap.Resource, texMap.UploadHeap));

	texMap.DescriptorIndex = SetTexDescriptor(texMap.Resource.Get());
	TextureList.push_back(move(texMap));

	return TextureList.back().DescriptorIndex;
}

int LoadTexture::LoadCure(const wchar_t* file)
{
	Texture texMap;
	texMap.Filename = file;
	ThrowIfFailed(CreateDDSTextureFromFile12(GetEngine()->GetDevice(),
		GetEngine()->GetCommandList(), file,
		texMap.Resource, texMap.UploadHeap));

	texMap.DescriptorIndex = SetCubeTexDescriptor(texMap.Resource.Get());
	TextureList.push_back(move(texMap));

	return TextureList.back().DescriptorIndex;
}

int LoadTexture::SetTexDescriptor(ID3D12Resource* tex)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	srvDesc.Format = tex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = tex->GetDesc().MipLevels;
	GetEngine()->GetDevice()->CreateShaderResourceView(tex, &srvDesc, GetEngine()->GetDescriptorHeap()->GetSrvDescriptorCpuHandle());
	return GetEngine()->GetDescriptorHeap()->GetSrvDescriptorIndex();
}

int LoadTexture::SetCubeTexDescriptor(ID3D12Resource* tex)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = tex->GetDesc().MipLevels;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	srvDesc.Format = tex->GetDesc().Format;
	GetEngine()->GetDevice()->CreateShaderResourceView(tex, &srvDesc, GetEngine()->GetDescriptorHeap()->GetSrvDescriptorCpuHandle());
	return GetEngine()->GetDescriptorHeap()->GetSrvDescriptorIndex();
}



