#pragma once
#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>

namespace Window
{


	void Create(const char* WindowName, int Width, int Height);
	void Show();
	void MsgLoop();
	void Destroy();
	LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
}
