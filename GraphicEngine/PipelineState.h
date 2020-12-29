#pragma once
#include "framework.h"

class PipelineState
{
public:
	void BuildPSO(string name, const wchar_t* file vs, const wchar_t* file ps, ComPtr<ID3D12RootSignature> signature);
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
	ShaderState mShaders;

};
