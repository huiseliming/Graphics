#pragma once
#include <memory>
class GameTimer;
class GameInput;

class Game
{
public:
	Game();
	Game(const Game& ) =delete;
	Game& operator=(const Game&) = delete;
	~Game();

	bool IsDone();

	void Update(float DeltaTime);

	void RenderScene();
	
	GameTimer* GetGameTimer() { return m_pGameTimer.get(); } 
private:
	std::unique_ptr<GameTimer> m_pGameTimer;
	std::unique_ptr<GameInput> m_pGameInput;
};



































