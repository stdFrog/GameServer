#include "pch.h"
#include "GameSessionManager.h"
#include "GameSession.h"

GameSessionManager GSessionManager;

void GameSessionManager::Add(std::shared_ptr<GameSession> NewSession) {
	std::lock_guard<std::mutex> WriteGuard(_Locks[0]);
	_Sessions.insert(NewSession);
}

void GameSessionManager::Remove(std::shared_ptr<GameSession> Target) {
	std::lock_guard<std::mutex> WriteGuard(_Locks[0]);
	_Sessions.erase(Target);
}

void GameSessionManager::Broadcast(std::shared_ptr<SendBuffer> Buffer) {
	std::lock_guard<std::mutex> WriteGuard(_Locks[0]);
	for (std::shared_ptr<GameSession> S : _Sessions) {
		S->Send(Buffer);
	}
}