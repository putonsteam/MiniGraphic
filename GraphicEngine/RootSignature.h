#pragma once
#include "framework.h"

class RootSignature
{
	friend D3DApp;
public:
	void BuildBaseRootSignature();
	ComPtr<ID3D12RootSignature> mBaseRootSignature;

private:
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();
};
