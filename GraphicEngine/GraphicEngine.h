#pragma once

#include "framework.h"
#include "FrameResource.h"
#include "LoadMaterial.h"
#include "LoadTexture.h"
#include "MeshInfo.h"
#include "RenderItem.h"
#include "DescriptorHeap.h"
#include "ShaderState.h"
#include "GameTimer.h"
#include "Camera.h"
#include "Macro.h"

static const int SwapChainBufferCount = 2;

class GraphicEngine
{
	GraphicEngine(); 
	DECLARE_SINGLE(GraphicEngine)
public:
	bool Init(int Width, int Height, HWND wnd, D3D_FEATURE_LEVEL level);
	void Run();
	void Flush();

	//camera
	void SetPosition(float x, float y, float z);
	void SetPosition(const DirectX::XMFLOAT3& v);
	DirectX::XMMATRIX GetView()const;
	DirectX::XMMATRIX GetProj()const;
	DirectX::XMVECTOR GetPosition()const;
	DirectX::XMFLOAT3 GetPosition3f()const;

	// Convenience overrides for handling mouse input.
	//void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);
	void OnKeyboardInput(const GameTimer& gt);

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
		const void* initData,
		UINT64 byteSize,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

	ComPtr<ID3D12Device> GetDevice() { return m_D3DDevice; }
	ComPtr<ID3D12GraphicsCommandList> GetCommandList() { return mCommandList; }
	LoadTexture& GetTextureList() { return TextureList; }
	DescriptorHeap* GetDescriptorHeap() { return mDescriptorHeap; }
	void InitDescriptorHeap(int size);
	ShaderState& GetShader() { return mShader; }
	FrameResource* GetFrameResource() { return mFrameResource; }
	ID3D12Fence* GetFence() { return m_Fence.Get(); }
	UINT64 GetCurrentFence() { return m_CurrentFence; }
	UINT64 IncreaseFence() { return ++m_CurrentFence; }
	void SendCommandAndFulsh();
	ID3D12Resource* CurrentBackBuffer()const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

public:
	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;
	// Derived class should set these in derived constructor to customize starting values.
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	int mClientWidth = 800;
	int mClientHeight = 600;

public:
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

	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
	int mCurrBackBuffer = 0;
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	HWND      mhMainWnd = nullptr; // main window handle
	Camera mCamera;
	LoadTexture TextureList;
	DescriptorHeap* mDescriptorHeap;
	ShaderState mShader;
	FrameResource* mFrameResource;
	POINT mLastMousePos;
};

extern GraphicEngine* GetEngine();
