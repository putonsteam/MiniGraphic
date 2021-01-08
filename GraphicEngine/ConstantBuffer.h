#pragma once
#include "framework.h"
#include "Lighting.h"
#include "Util.h"
#include "MathHelper.h"
#include "UploadBuffer.h"
#include "GraphicEngine.h"

#define MAX_CONSTENT_BUFFER_SIZE 3

// Stores the resources needed for the CPU to build the command lists
// for a frame.  
template<class T>
class ConstantBuffer
{
public:

	ConstantBuffer(UINT elementCount);
	//     ConstantBuffer(const ConstantBuffer& rhs) = delete;
	//     ConstantBuffer& operator=(const ConstantBuffer& rhs) = delete;
	~ConstantBuffer();
	void Update(int elementIndex, const T& data);
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress();

	// We cannot reset the allocator until the GPU is done processing the commands.
	// So each frame needs their own allocator.
	//ComPtr<ID3D12CommandAllocator> CmdListAlloc;
	ID3D12Resource* Resource()const
	{
		return CBuffer[CurrentSize]->Resource();
	}


private:
	// We cannot update a cbuffer until the GPU is done processing the commands
	// that reference it.  So each frame needs their own cbuffers.
	vector<unique_ptr<UploadBuffer<T>>> CBuffer;

	// Fence value to mark commands up to this fence point.  This lets us
	// check if these frame resources are still in use by the GPU.
	//UINT64 Fence = 0;
	int CurrentSize = 0;
};

template<class T>
ConstantBuffer<T>::ConstantBuffer(UINT elementCount)
{
	for (int i = 0; i != MAX_CONSTENT_BUFFER_SIZE; ++i)
	{
		CBuffer.push_back(make_unique<UploadBuffer<T>>(GetEngine()->GetDevice(), elementCount, true));
	}
}

template<class T>
ConstantBuffer<T>::~ConstantBuffer()
{

}

template<class T>
void ConstantBuffer<T>::Update(int elementIndex, const T& data)
{
	if (++CurrentSize >= MAX_CONSTENT_BUFFER_SIZE)
	{
		CurrentSize = 0;
	}
	CBuffer[CurrentSize]->CopyData(elementIndex, data);
}

template<class T>
D3D12_GPU_VIRTUAL_ADDRESS ConstantBuffer<T>::GetGPUAddress()
{
	CBuffer[CurrentSize]->Resource()->GetGPUVirtualAddress();
}