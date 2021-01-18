#pragma once
#include "framework.h"

class LoadTexture
{
public:
	int Load(const wchar_t* file);
	int LoadCure(const wchar_t* file);
	int SetTexDescriptor(ID3D12Resource* tex);
	int SetCubeTexDescriptor(ID3D12Resource* tex);

	struct Texture
	{
		// Unique material name for lookup.
		string Name;
		wstring Filename;
		ComPtr<ID3D12Resource> Resource = nullptr;
		ComPtr<ID3D12Resource> UploadHeap = nullptr;
		int DescriptorIndex;
	};

	vector<Texture> TextureList;

};
