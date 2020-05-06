#pragma once
#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>
#include <string>

//窗口对象的简单封装
class Window
{
public:
	Window();
	Window(const wchar_t* WindowName, int Width, int Height , LRESULT (CALLBACK *WndProc)(HWND, UINT, WPARAM, LPARAM) = DefaultWndProc);
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
	virtual ~Window();

	void Create(const wchar_t* WindowName, int Width, int Height, LRESULT(CALLBACK* WndProc)(HWND, UINT, WPARAM, LPARAM) = DefaultWndProc);
	void Destroy();
	void Show();
	bool Resize(int Width, int Height);

	static void MsgLoop();
	static LRESULT WINAPI DefaultWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	HWND GetHWND() { return m_hWnd; }
	int GetWidth(){ return m_Width; }
	void SetWidth(int Width) { m_Width = Width; }
	int GetHeight() { return m_Height; }
	void SetHeight(int Height) { m_Height = Height; }
	bool GetActivate() { return m_bIsActivate; }
	void SetActivate(bool bIsActivate) { m_bIsActivate = bIsActivate; }

private:
	int m_Width = 0;
	int m_Height = 0;
	HWND m_hWnd = NULL;
	WNDCLASSEX m_WndClass = {};
	bool m_bIsActivate;// ACTIVATE
};
