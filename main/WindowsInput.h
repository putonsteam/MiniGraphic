#pragma once

#include "framework.h"
#include "resource.h"

class WindowsInput
{
public:
	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static WindowsInput* GetInstance();

	~WindowsInput();

private:
	static WindowsInput* m_pInstance;

};