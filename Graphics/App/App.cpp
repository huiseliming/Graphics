#include "App.h"
#include "../Graphics/Graphics.h"
#include "../Utility/Utility.h"
#include "../Engine/Game.h"



App::App()
{
	m_pWindow = std::make_unique<Window>();
	m_pWindow->Create(L"MyGraphics", 800, 600);
	m_pGraphics = std::make_unique<Graphics>();
	m_pGame = std::make_unique<Game>();

}

App::~App()
{

}

bool App::Update()
{
	float DeltaTime = m_pGraphics->GetFrameTime();


	m_pGame->Update(DeltaTime);
	m_pGame->RenderScene();

	//m_pGraphics->Present();

	return !m_pGame->IsDone();
}

void App::Run()
{
	m_pWindow->Show();
	MSG Msg = {};
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
	m_pGraphics->Terminate();
}



int main(int argc, char* argv[])
{
	try
	{
		App app;
		GlobalVariable<App>::Set(&app);
		app.Run();
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
