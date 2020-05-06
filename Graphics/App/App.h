#pragma once
#include <memory>

class Game;
class Graphics;
class Window;

class App
{
public:
	App();

	virtual ~App();

	bool Update();

	void Run();

private:
	std::unique_ptr<Game> m_pGame;
	std::unique_ptr<Graphics> m_pGraphics;
	std::unique_ptr<Window> m_pWindow;
};























