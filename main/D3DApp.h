#pragma once
#include "framework.h"

class D3DApp
{
public:
	bool Init(int Width, int Height, HWND wnd);
	void Run();

private:
	void LoadMesh();

	//GraphicEngine engine;
};
