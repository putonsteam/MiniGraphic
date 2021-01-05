#pragma once
#include "framework.h"
#include "ShaderState.h"
#include "GameTimer.h"
#include "ConstantBuffer.h"
#include "FrameResource.h"
#include "RenderItem.h"
#include "Lighting.h"

class D3DApp
{
public:
	D3DApp() {}
	bool Init(int Width, int Height, HWND wnd);
	void Run();
	void Draw(const GameTimer& Timer);
	void Update(const GameTimer& Timer);
	void UpdateObjectCBs(const GameTimer& Timer);
	void UpdateMaterialBuffer(const GameTimer& Timer);
	//void UpdateShadowTransform(const GameTimer& Timer);
	void UpdateMainPassCB(const GameTimer& Timer);
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<unique_ptr<RenderItem>>& ritems);
	void BuildPSO(const wchar_t* vsFile, const wchar_t* psFile);
	void BuildSkyPSO(const wchar_t* vsFile, const wchar_t* psFile);

	

	struct ObjectConstants
	{
		DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
		UINT     MaterialIndex;
 		UINT     ObjPad0;
 		UINT     ObjPad1;
 		UINT     ObjPad2;
	};

	struct PassConstants
	{
// 		DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
// 		DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
// 		DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
// 		DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
		//DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
		DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
		float cbPerObjectPad1 = 0.0f;
// 		DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
// 		DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
// 		float NearZ = 0.0f;
// 		float FarZ = 0.0f;
// 		float TotalTime = 0.0f;
// 		float DeltaTime = 0.0f;

		DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

		// Indices [0, NUM_DIR_LIGHTS) are directional lights;
		// indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
		// indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
		// are spot lights for a maximum of MaxLights per object.
		Light Lights[MaxLights];
	};

	struct MaterialData
	{
		DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
		float Roughness = 0.5f;

		// Used in texture mapping.
		DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();

		UINT DiffuseMapIndex = 0;
 		UINT MaterialPad0;
 		UINT MaterialPad1;
 		UINT MaterialPad2;
	};

	// Lightweight structure stores parameters to draw a shape.  This will
// vary from app-to-app.
	enum class RenderLayer : int
	{
		Opaque = 0,
		Debug,
		Sky,
		Count
	};

private:
	array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();
	void BuildBaseRootSignature();
	void LoadRenderItem();
	ComPtr<ID3D12RootSignature> mBaseRootSignature;

	std::unique_ptr<ConstantBuffer<PassConstants>> PassCB = nullptr;
	std::unique_ptr<ConstantBuffer<ObjectConstants>> ObjectCB = nullptr;

	std::unique_ptr<ConstantBuffer<MaterialData>> MaterialBuffer = nullptr;

	std::vector<unique_ptr<RenderItem>> mRitemLayer[(int)RenderLayer::Count];
	FrameResource* mCurrFrameResource;
	float mLightRotationAngle = 0.0f;
	XMFLOAT3 mBaseLightDirections[3] = {
		XMFLOAT3(0.57735f, -0.57735f, 0.57735f),
		XMFLOAT3(-0.57735f, -0.57735f, 0.57735f),
		XMFLOAT3(0.0f, -0.707f, -0.707f)
	};
	XMFLOAT3 mRotatedLightDirections[3];
	PassConstants mMainPassCB;  // index 0 of pass cbuffer.
	CD3DX12_GPU_DESCRIPTOR_HANDLE mNullSrv;
	ComPtr<ID3D12PipelineState> mBasePSO;
	ComPtr<ID3D12PipelineState> mSkyPSO;
};


