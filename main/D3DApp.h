#pragma once
#include "framework.h"
#include "ShaderState.h"
#include "PipelineState.h"

class D3DApp
{
public:
	bool Init(int Width, int Height, HWND wnd);
	void Run();

private:
	void LoadRenderItem();
	PipelineState mPSO;
	LoadMaterial mMaterial;
	//GraphicEngine engine;
};
