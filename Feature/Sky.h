#pragma once
#include "GraphicEngine.h"

class Sky
{
public:
	void BuildSkyPSO(const wchar_t* vsFile, const wchar_t* psFile);
	//void BuildBaseRootSignature();
	void LoadRenderItem();
	void Draw(const GameTimer& Timer);
	UINT GetSkyHeapIndex() { return mSkyTexHeapIndex; }
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetSkyHeapStart();

private:
	ComPtr<ID3D12PipelineState> mSkyPSO;
	UINT mSkyTexHeapIndex = 0;

};

