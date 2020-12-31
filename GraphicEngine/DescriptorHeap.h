#pragma once
#include "framework.h"

class DescriptorHeap
{
public:
	DescriptorHeap(int size);
	int DistributeTexDescriptor(ID3D12Resource* tex);

public:
	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
	int HeapSize;
	int Index;
};
