#pragma once

#include "framework.h"

class GraphicEngine
{
	DECLARE_SINGLE(GraphicEngine)
public:
	bool Init(int Width, int Height, HWND wnd, D3D_FEATURE_LEVEL level);
	void Run();
	void Flush();
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
		const void* initData,
		UINT64 byteSize,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

	ComPtr<ID3D12Device> GetDevice() { return m_D3DDevice; }
	ComPtr<ID3D12GraphicsCommandList> GetCommandList() { return mCommandList; }

public:
	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;
	// Derived class should set these in derived constructor to customize starting values.
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	int mClientWidth = 800;
	int mClientHeight = 600;

private:
	bool InitDevice();
	void InitGPUCommand();
	void InitDesHeap();
	void InitSwapchainAndRvt();
	void InitDsv();
	void InitViewportAndScissor();


	Microsoft::WRL::ComPtr<IDXGIFactory4>               m_DxgiFactory;
	std::wstring                                        m_AdapterDescription;
	ComPtr<IDXGIAdapter1>                               m_Adapter;
	Microsoft::WRL::ComPtr<ID3D12Device>                m_D3DDevice;
	D3D_FEATURE_LEVEL                                   m_D3DMinFeatureLevel;

	Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
	UINT64 m_CurrentFence = 0;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

	static const int SwapChainBufferCount = 2;
	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
	int mCurrBackBuffer = 0;
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	HWND      mhMainWnd = nullptr; // main window handle
};

extern GraphicEngine* GetEngine();
