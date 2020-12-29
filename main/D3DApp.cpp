#include "D3DApp.h"
#include "LoadTextMesh.h"
#include "GraphicEngine.h"
#include "LoadTexture.h"
#include "LoadMaterial.h"


bool D3DApp::Init(int Width, int Height, HWND wnd)
{
	GetEngine()->Init(Width, Height, wnd, D3D_FEATURE_LEVEL_11_0);

	LoadRenderItem();
	return true;
}

void D3DApp::LoadRenderItem()
{
	LoadTextMesh::Load("source/Models/skull.txt");

	//auto skullMat = std::make_unique<LoadMaterial>();
	mMaterial.AddMaterial("skullMat", 3,
		XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f),
		XMFLOAT3(0.6f, 0.6f, 0.6f),
		0.2f);

// 	LoadTexture tex;
	mMaterial.LoadTexture("skullMat", L"text");

	mPSO.BuildPSO("opaque", L"Shaders\\Default.hlsl", L"Shaders\\Default.hlsl", );
	//LoadMaterial();
	//LoadLighting();
}



void D3DApp::Run()
{

}