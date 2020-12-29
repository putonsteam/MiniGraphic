#pragma once
#include "framework.h"

class ShaderState
{
public:
	ShaderState();
	ComPtr<ID3DBlob> CreateVSShader(string name, const wchar_t* file);
	ComPtr<ID3DBlob> CreatePSShader(string name, const wchar_t* file);
	ComPtr<ID3DBlob> CompileShader(const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target);
	std::vector < D3D12_INPUT_ELEMENT_DESC>& GetLayout() { return mInputLayout; }
	ComPtr<ID3DBlob> GetShader(string &str) { return mShaders[str]; }

private:
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
};