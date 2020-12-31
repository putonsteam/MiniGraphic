#pragma once
#include "framework.h"

class DescriptorHeap
{
public:
	DescriptorHeap(int size);
	int DistributeTexDescriptor(ID3D12Resource* tex);
	ID3D12DescriptorHeap* GetSrvDescHeap() { return mSrvDescriptorHeap.Get(); }

private:
	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
	int HeapSize;
	int Index;
};
