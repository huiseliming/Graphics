#include "Game.h"
#include "GameInput.h"
#include "SystemTime.h"
#include "../Utility/Utility.h"
#include "../Graphics/CommandContext.h"

Game::Game()
{
	GlobalVariable<Game>::Set(this);
	m_pGameTimer = std::make_unique<GameTimer>();
	m_pGameInput = std::make_unique<GameInput>();
}

Game::~Game()
{

}

void Game::Begin()
{
	m_pGameTimer->Reset();
}

void Game::End()
{
	m_pGameTimer->Stop();
}

bool Game::IsDone()
{
	return m_pGameInput->IsPressed(GameInput::kKey_escape);
}

void Game::Update(float DeltaTime)
{
	m_pGameInput->Update(DeltaTime);
}

void Game::RenderScene()
{
}
