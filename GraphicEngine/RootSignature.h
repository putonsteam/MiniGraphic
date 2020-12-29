#pragma once

class RootSignature
{
	void BuildRootSignature();
	std::unordered_map<std::string, ComPtr<ID3D12RootSignature>> mRootSignature;

};
