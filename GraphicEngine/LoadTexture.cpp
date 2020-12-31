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

	texMap.DescriptorIndex = GetEngine()->GetDescriptorHeap()->DistributeTexDescriptor(texMap.Resource.Get());
	TextureList.push_back(texMap);

	return texMap.DescriptorIndex;
}


