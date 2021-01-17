#include "DescriptorHeap.h"
#include "GraphicEngine.h"

void DescriptorHeap::CreateSrvDescriptorHeap(int size)
{
	SrvSize = size;
	SrvIndex = -1;
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = size;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(GetEngine()->GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));
}

void DescriptorHeap::CreateRtvDescriptorHeap(int size)
{
	RtvSize = size;
	RtvIndex = -1;
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = size;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(GetEngine()->GetDevice()->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvDescriptorHeap.GetAddressOf())));
}

void DescriptorHeap::CreateDsvDescriptorHeap(int size)
{
	DsvSize = size;
	DsvIndex = -1;
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = size;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(GetEngine()->GetDevice()->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(mDsvDescriptorHeap.GetAddressOf())));
}

int DescriptorHeap::SetDsvDescriptorIndex()
{
	return ++DsvIndex;
}

int DescriptorHeap::SetSrvDescriptorIndex()
{
	return ++SrvIndex;
}

int DescriptorHeap::SetRtvDescriptorIndex()
{
	return ++RtvIndex;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetSrvDescriptorCpuHandle()
{
	return GetSrvDescriptorCpuHandle(SetSrvDescriptorIndex());
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetRtvDescriptorCpuHandle()
{
	return GetRtvDescriptorCpuHandle(SetRtvDescriptorIndex());
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetDsvDescriptorCpuHandle()
{
	return GetDsvDescriptorCpuHandle(SetDsvDescriptorIndex());
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetSrvDescriptorGpuHandle(int offset)
{
	CD3DX12_GPU_DESCRIPTOR_HANDLE descriptor(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	descriptor.Offset(offset, GetEngine()->mCbvSrvUavDescriptorSize);
	return descriptor;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetSrvDescriptorCpuHandle(int offset)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE descriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	descriptor.Offset(offset, GetEngine()->mCbvSrvUavDescriptorSize);
	return descriptor;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetRtvDescriptorGpuHandle(int offset)
{
	CD3DX12_GPU_DESCRIPTOR_HANDLE descriptor(mRtvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	descriptor.Offset(offset, GetEngine()->mCbvSrvUavDescriptorSize);
	return descriptor;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetRtvDescriptorCpuHandle(int offset)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE descriptor(mRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	descriptor.Offset(offset, GetEngine()->mRtvDescriptorSize);
	return descriptor;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetDsvDescriptorGpuHandle(int offset)
{
	CD3DX12_GPU_DESCRIPTOR_HANDLE descriptor(mDsvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	descriptor.Offset(offset, GetEngine()->mDsvDescriptorSize);
	return descriptor;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetDsvDescriptorCpuHandle(int offset)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE descriptor(mDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	descriptor.Offset(offset, GetEngine()->mCbvSrvUavDescriptorSize);
	return descriptor;
}

DescriptorHeap::DescriptorHeap()
{

}



