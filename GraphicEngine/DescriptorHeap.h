#pragma once
#include "framework.h"

class DescriptorHeap
{
public:
	DescriptorHeap();
	void CreateSrvDescriptorHeap(int size);
	void CreateRtvDescriptorHeap(int size);
	void CreateDsvDescriptorHeap(int size);
	int SetDsvDescriptorIndex();
	int SetSrvDescriptorIndex();
	int SetRtvDescriptorIndex();
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetSrvDescriptorCpuHandle();
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtvDescriptorCpuHandle();
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDsvDescriptorCpuHandle();
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrvDescriptorGpuHandle(int offset);
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetSrvDescriptorCpuHandle(int offset);
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetRtvDescriptorGpuHandle(int offset);
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtvDescriptorCpuHandle(int offset);
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetDsvDescriptorGpuHandle(int offset);
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDsvDescriptorCpuHandle(int offset);
	ID3D12DescriptorHeap* GetSrvDescHeap() { return mSrvDescriptorHeap.Get(); }
	ID3D12DescriptorHeap* GetRtvDescHeap() { return mRtvDescriptorHeap.Get(); }
	ID3D12DescriptorHeap* GetDsvDescHeap() { return mDsvDescriptorHeap.Get(); }
	int GetSrvDescriptorIndex() { return SrvIndex; }
	int GetRtvDescriptorIndex() { return RtvIndex; }
	int GetDsvDescriptorIndex() { return DsvIndex; }

private:
	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> mRtvDescriptorHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> mDsvDescriptorHeap = nullptr;
	int SrvSize;
	int SrvIndex;
	int RtvSize;
	int RtvIndex;
	int DsvSize;
	int DsvIndex;
};
