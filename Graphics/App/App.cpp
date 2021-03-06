#include "App.h"
#include "../Utility/Utility.h"
#include "../Graphics/Graphics.h"
#include "../Engine/Game.h"
#include "../Graphics/ImGui/imgui.h"
#include "../Graphics/ImGui/imgui_impl_win32.h"
#include "../Graphics/ImGui/imgui_impl_dx12.h"

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

App::App()
{
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontDefault();
	//ImFont* font = io.Fonts->AddFontFromFileTTF(D3D12Helper::ModuleFilePathAppendFile<std::string,std::string>("font.ttf").c_str(), 24.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
	//ImGui::GetIO().FontDefault = font;
	ImGui::StyleColorsDark();

	GlobalVariable<App>::Set(this);
	m_pWindow = std::make_unique<Window>();
	m_pWindow->Create(L"MyGraphics", 800, 600, WndProc);
	m_pGraphics = std::make_unique<Graphics>();
	m_pGame = std::make_unique<Game>();
}

App::~App()
{
}

bool App::Update()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	float DeltaTime = m_pGraphics->GetFrameTime();

	m_pGame->Update(DeltaTime);
	m_pGraphics->WaitForNextFrame();
	m_pGraphics->RenderScene();
	m_pGraphics->RenderUI();
	m_pGraphics->Present();
	return !m_pGame->IsDone();
}

void App::Run()
{
	m_pWindow->Show();
	MSG Msg = {};
	m_pGame->Begin();
	while (Msg.message != WM_QUIT)
	{
		if (::PeekMessage(&Msg, NULL, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&Msg);
			//将消息发给窗口消息处理函数处理
			::DispatchMessage(&Msg);
			continue;
		}

		if (!Update())
			break;
	}
	m_pGame->End();
	m_pGraphics->Terminate();
}

extern IMGUI_IMPL_API LRESULT  ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;
	switch (msg)
	{
	case WM_SIZE: //大小改变
		GlobalVariable<Graphics>::Get()->Resize((UINT)LOWORD(lParam) , (UINT)HIWORD(lParam));
		GlobalVariable<Window>::Get()->SetWidth((UINT)LOWORD(lParam));
		GlobalVariable<Window>::Get()->SetHeight((UINT)HIWORD(lParam));
		//if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
		//{
		//	ImGui_ImplDX12_InvalidateDeviceObjects();
		//	CleanupRenderTarget();
		//	ResizeSwapChain(hWnd, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam));
		//	CreateRenderTarget();
		//	ImGui_ImplDX12_CreateDeviceObjects();
		//}
		return 0;
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			GlobalVariable<Window>::Get()->SetActivate(false);
		}
		else
		{
			GlobalVariable<Window>::Get()->SetActivate(true);
		}
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

int main(int argc, char* argv[])
{
	try
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		App* app = new App;
		app->Run();
		delete app;
		ImGui::DestroyContext();
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
