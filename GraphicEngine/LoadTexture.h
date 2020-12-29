#pragma once
#include "framework.h"

class LoadTexture
{
public:
	int Load(const wchar_t* file);
	void BuildTextureDescriptorHeaps();

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
	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
};
