#include "D3DApp.h"
#include "LoadTextMesh.h"

bool D3DApp::Init(int Width, int Height, HWND wnd)
{
	GetEngine()->Init(Width, Height, wnd, D3D_FEATURE_LEVEL_11_0);

	LoadMesh();
	return true;
}

void D3DApp::LoadMesh()
{
	LoadTextMesh::Load("source/Models/skull.txt");
}

void D3DApp::Run()
{

}