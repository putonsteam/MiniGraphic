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
#include "ConstantBuffer.h"

static const int SwapChainBufferCount = 2;

class GraphicEngine
{
	DECLARE_SINGLE(GraphicEngine)

public:
	GraphicEngine();
	bool Init(int Width, int Height, HWND wnd, D3D_FEATURE_LEVEL level);
	void InitDescriptorHeap(int size);
	void Run();
	void Update(const GameTimer& Timer);
	void Flush();

	//camera
	void SetPosition(float x, float y, float z);
	void SetPosition(const XMFLOAT3& v);
	XMMATRIX GetView()const;
	XMMATRIX GetProj()const;
	XMVECTOR GetPosition()const;
	XMFLOAT3 GetPosition3f()const;

	// Convenience overrides for handling mouse input.
	//void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);
	void OnKeyboardInput();

	ComPtr<ID3D12Resource> CreateDefaultBuffer(
		const void* initData,
		UINT64 byteSize,
		ComPtr<ID3D12Resource>& uploadBuffer);

	ComPtr<ID3D12Resource> CreateArray2DBuffer(
		D3D12_RESOURCE_DESC& texDesc,
		const void* initData,
		UINT64 byteSize,
		ComPtr<ID3D12Resource>& uploadBuffer);
	ID3D12Device* GetDevice() { return m_D3DDevice.Get(); }
	ID3D12CommandQueue* GetCommandQueue() { return mCommandQueue.Get(); }
	ID3D12CommandAllocator* GetCommandAlloc() { return mDirectCmdListAlloc.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() { return mCommandList.Get(); }
	D3D12_VIEWPORT* GetViewport() { return &mScreenViewport; }
	D3D12_RECT* GetScissor() { return &mScissorRect; }
	IDXGISwapChain* GetSwapChain() { return mSwapChain.Get(); }
	LoadTexture* GetTextureList() { return &TextureList; }
	DescriptorHeap* GetDescriptorHeap() { return mDescriptorHeap; }
	ShaderState* GetShader() { return &mShader; }
	Camera* GetCamera() { return &mCamera; }
	FrameResource* GetFrameResource() { return mFrameResource; }
	ID3D12DescriptorHeap* GetSrvDescHeap() { return GetDescriptorHeap()->GetSrvDescHeap(); }
	ID3D12Fence* GetFence() { return m_Fence.Get(); }
	UINT64 GetCurrentFence() { return m_CurrentFence; }
	GameTimer& GetTimer() { return mTimer; }
	ID3D12RootSignature* GetBaseRootSignature() { return mBaseRootSignature.Get(); }

	UINT64 IncreaseFence() { return ++m_CurrentFence; }
	void SendCommandAndFulsh();
	int GetCurrBackBufferIndex() { return mCurrBackBufferIndex; }
	void SetCurrBackBufferIndex()
	{
		mCurrBackBufferIndex = (mCurrBackBufferIndex + 1) % SwapChainBufferCount;
	}
	ID3D12Resource* CurrentBackBuffer()const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

	void DrawRenderItems(RenderLayer layer/*ID3D12GraphicsCommandList* cmdList, *//*const std::vector<unique_ptr<RenderItem>>& ritems*/);
	void UpdateObjectCBs(const GameTimer& Timer);
	void UpdateMaterialBuffer(const GameTimer& Timer);
	//void UpdateShadowTransform(const GameTimer& Timer);
	void UpdateMainPassCB(const GameTimer& Timer);
	void UpdateShaderParameter(const GameTimer& Timer);
	void CreateShaderParameter();
	void AddRenderItem(RenderLayer layer, unique_ptr<RenderItem>& item);
	void BuildBaseRootSignature();
	array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();

	void SetBaseRootSignature0();
	void SetBaseRootSignature1();
	void SetBaseRootSignature2();

	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;
	// Derived class should set these in derived constructor to customize starting values.
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	int mClientWidth = 800;
	int mClientHeight = 600;
	PassConstants* GetMainPassCb() { return &mMainPassCB; }

private:
	bool InitDevice();
	void InitGPUCommand();
	void InitDesHeap();
	void InitSwapchainAndRvt();
	void InitDsv();
	void InitViewportAndScissor();
	void CalculateFrameStats();

	ComPtr<IDXGIFactory4>               m_DxgiFactory;
	std::wstring                                        m_AdapterDescription;
	ComPtr<IDXGIAdapter1>                               m_Adapter;
	ComPtr<ID3D12Device>                m_D3DDevice;
	D3D_FEATURE_LEVEL                                   m_D3DMinFeatureLevel;

	ComPtr<ID3D12Fence> m_Fence;
	UINT64 m_CurrentFence = 0;

	ComPtr<ID3D12CommandQueue> mCommandQueue;
	ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
	ComPtr<ID3D12GraphicsCommandList> mCommandList;

	ComPtr<IDXGISwapChain> mSwapChain;
	int mCurrBackBufferIndex = 0;
	ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
	//ComPtr<ID3D12DescriptorHeap> mRtvHeap;

	//ComPtr<ID3D12DescriptorHeap> mDsvHeap;
	ComPtr<ID3D12Resource> mDepthStencilBuffer;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	HWND      mhMainWnd = nullptr; // main window handle
	Camera mCamera;
	LoadTexture TextureList;
	DescriptorHeap* mDescriptorHeap;
	ShaderState mShader;
	FrameResource* mFrameResource;
	POINT mLastMousePos;
	GameTimer mTimer;
	std::unique_ptr< ConstantBuffer<PassConstants> > PassCB;
	std::unique_ptr< ConstantBuffer<ObjectConstants> > ObjectCB;
	std::unique_ptr< ConstantBuffer<MaterialData> > MaterialBuffer;
	PassConstants mMainPassCB;  // index 0 of pass cbuffer.
	std::vector<unique_ptr<RenderItem>> mRitemLayer[(int)RenderLayer::Count];
	ComPtr<ID3D12RootSignature> mBaseRootSignature;

};

extern GraphicEngine* GetEngine();
