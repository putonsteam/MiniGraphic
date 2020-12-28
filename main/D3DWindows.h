#pragma once

#include "framework.h"

#define MAX_LOADSTRING 100

class D3DWindows
{
public:
	bool InitWindows(HINSTANCE hInstance, int nCmdShow, int width, int height);
	HWND GetWnd() { return hWnd; }

private:
	ATOM MyRegisterClass();

	// Global Variables:
	HINSTANCE hInst;                                // current instance
	WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
	WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
	HWND hWnd;
};
