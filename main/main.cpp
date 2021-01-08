#include "framework.h"
#include "D3DWindows.h"
#include "D3DApp.h"
#include "Game.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	int Width = 800;
	int Height = 600;
	D3DWindows window;
	window.InitWindows(hInstance, nCmdShow, Width, Height);

	D3DApp app;
	app.Init(Width, Height, window.GetWnd());

	if (!CGame::GetInstance()->Initialize(hInstance, window.GetWnd()))
	{
		return FALSE;
	}

	MSG msg = { 0 };

	// Main message loop:
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			//app.Run();
			CGame::GetInstance()->Run();
		}
	}

	return (int)msg.wParam;
}