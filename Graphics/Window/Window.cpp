#include "Window.h"
#include <tchar.h>
#include "../resource.h"
#include "../Tool/Exception.h"

#include "../Graphics/ImGui/imgui.h"
#include "../Graphics/ImGui/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Window
{
	int g_Width = 0;
	int g_Height = 0;
	HWND g_hWnd = NULL;
	WNDCLASSEX g_WndClass = {};
	std::wstring g_ModulePath;
	void Create(const wchar_t* WindowName, int Width, int Height)
	{
		g_Width = Width;
		g_Height = Height;
		//创建窗口类
		g_WndClass.cbSize = sizeof(WNDCLASSEX);
		g_WndClass.style = CS_CLASSDC;
		g_WndClass.lpfnWndProc = WndProc;
		g_WndClass.hInstance = GetModuleHandle(NULL);
		g_WndClass.hIcon = static_cast<HICON>(LoadImage(g_WndClass.hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 32, 32, 0));
		g_WndClass.lpszClassName = WindowName;
		g_WndClass.hIconSm = static_cast<HICON>(LoadImage(g_WndClass.hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 32, 32, 0));
		//注册窗口类
		::RegisterClassEx(&g_WndClass);
		//使用窗口类创建窗口
		g_hWnd = ::CreateWindow(g_WndClass.lpszClassName, WindowName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, Width, Height, NULL, NULL, g_WndClass.hInstance, NULL);
		//初始化路径
		WCHAR Path[MAX_PATH];
		THROW_LAST_EXCEPT(0 != GetModuleFileName(NULL, Path, MAX_PATH));
		g_ModulePath = Path;

		//初始化ImGui
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->AddFontDefault();
		//ImFont* font = io.Fonts->AddFontFromFileTTF(D3D12Helper::ModuleFilePathAppendFile<std::string,std::string>("font.ttf").c_str(), 24.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
		//ImGui::GetIO().FontDefault = font;
		ImGui::StyleColorsDark();
		ImGui_ImplWin32_Init(g_hWnd);

	}

	void Show()
	{
		::ShowWindow(g_hWnd, SW_SHOWDEFAULT);
		::UpdateWindow(g_hWnd);
	}

	void MsgLoop()
	{
		MSG Msg;
		ZeroMemory(&Msg, sizeof(Msg));
		while (Msg.message != WM_QUIT)
		{
			if (::PeekMessage(&Msg, NULL, 0U, 0U, PM_REMOVE))
			{
				::TranslateMessage(&Msg);
				//将消息发给窗口消息处理函数处理
				::DispatchMessage(&Msg);
				continue;
			}
			if (false)
				break;
		}
	}

	void Destroy()
	{
		//销毁ImGui
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		//销毁窗口
		::DestroyWindow(g_hWnd);
		::UnregisterClass(g_WndClass.lpszClassName, g_WndClass.hInstance);
	}

	LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
			return true;

		switch (msg)
		{
		case WM_SIZE: //大小改变
			//if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
			//{
			//	ImGui_ImplDX12_InvalidateDeviceObjects();
			//	CleanupRenderTarget();
			//	ResizeSwapChain(hWnd, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam));
			//	CreateRenderTarget();
			//	ImGui_ImplDX12_CreateDeviceObjects();
			//}
			return 0;
		case WM_SYSCOMMAND: //系统命令
			if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
				return 0;
			break;
		case WM_DESTROY:
			::PostQuitMessage(0);
			return 0;
		}
		return ::DefWindowProc(hWnd, msg, wParam, lParam);
	}
}

int main(int argc, char* argv[])
{
	try
	{
		Window::Create(L"GreyDawnGraphics",800,600);
		Window::Show();
		Window::MsgLoop();
		Window::Destroy();
		return 0;
	}
	catch (const Exception& e)
	{
		MessageBoxA(nullptr, e.what(), e.GetType(), MB_OK | MB_ICONEXCLAMATION);
	}
	catch (const std::exception& e)
	{
		MessageBoxA(nullptr, e.what(), "Standard Exception", MB_OK | MB_ICONEXCLAMATION);
	}
	catch (...)
	{
		MessageBoxA(nullptr, "No details available", "Unknown Exception", MB_OK | MB_ICONEXCLAMATION);
	}
	return -1;
}