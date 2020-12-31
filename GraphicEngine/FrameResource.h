#pragma once
#include "framework.h"
#include "d3dUtil.h"
#include "MathHelper.h"
#include "UploadBuffer.h"

const int gNumFrameResources = 3;
// Stores the resources needed for the CPU to build the command lists
// for a frame.  

class FrameResource
{
public:

	FrameResource();
	~FrameResource();
	void Init();

	// We cannot reset the allocator until the GPU is done processing the commands.
	// So each frame needs their own allocator.

	UINT64 GetFence() { return Fence[mCurrFrameResourceIndex]; }
	void SetFence(UINT64 value) { Fence[mCurrFrameResourceIndex] = value; }
	ComPtr<ID3D12CommandAllocator> GetCurrentCommandAllocator();

	void Update();


private:
	ComPtr<ID3D12CommandAllocator> CmdListAlloc[gNumFrameResources];

    // Fence value to mark commands up to this fence point.  This lets us
    // check if these frame resources are still in use by the GPU.
	UINT64 Fence[gNumFrameResources] = { 0 };
	int mCurrFrameResourceIndex = 0;

};