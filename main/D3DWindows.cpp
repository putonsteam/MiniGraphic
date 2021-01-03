// D3DWindows.cpp : Defines the entry point for the application.
//

#include "D3DWindows.h"
#include "WindowsInput.h"

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return WindowsInput::GetInstance()->WndProc(hWnd, message, wParam, lParam);
}

bool D3DWindows::InitWindows(HINSTANCE hInstance, int nCmdShow, int width, int height)
{
    // TODO: Place code here.

	hInst = hInstance; // Store instance handle in our global variable

    // Initialize global strings
    //LoadStringW(hInst, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    //LoadStringW(hInst, IDC_MINIGRAPHIC, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass();

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT R = { 0, 0, width, height };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	width = R.right - R.left;
	height = R.bottom - R.top;

	std::wstring mMainWndCaption = L"d3d App";

	hWnd = CreateWindow(L"MainWnd", mMainWndCaption.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, hInst, 0);
	if (!hWnd)
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM D3DWindows::MyRegisterClass()
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInst;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"MainWnd";

	return RegisterClass(&wc);
}
