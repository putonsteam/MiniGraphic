#include "FrameResource.h"
#include "GraphicEngine.h"


//const int gNumFrameResources = 3;

FrameResource::FrameResource()
{

}

void FrameResource::Init()
{
	for (int i = 0; i != gNumFrameResources; ++i)
	{
		ThrowIfFailed(GetEngine()->GetDevice()->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(CmdListAlloc[i].GetAddressOf())));
	}
}

FrameResource::~FrameResource()
{
	
}

void FrameResource::Update()
{
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
}

ComPtr<ID3D12CommandAllocator> FrameResource::GetCurrentCommandAllocator()
{
	return CmdListAlloc[mCurrFrameResourceIndex];
}

