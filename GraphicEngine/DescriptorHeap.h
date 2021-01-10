#pragma once
#include "framework.h"

class DescriptorHeap
{
public:
	DescriptorHeap(int size);
	int DistributeTexDescriptor(ID3D12Resource* tex);
	ID3D12DescriptorHeap* GetSrvDescHeap() { return mSrvDescriptorHeap.Get(); }
	int DistributeCubeDescriptor(ID3D12Resource* tex);
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetGpuDescHandle(int offset);

private:
	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
	int HeapSize;
	int Index;
};
