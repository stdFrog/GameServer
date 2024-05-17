#pragma once

class GameSession;

class GameSessionManager
{
	std::mutex _Locks[100];
	std::set<std::shared_ptr<GameSession>> _Sessions;

public:
	void Add(std::shared_ptr<GameSession> NewSession);
	void Remove(std::shared_ptr<GameSession> Target);
	void Broadcast(std::shared_ptr<SendBuffer> Buffer);
};

extern GameSessionManager GSessionManager;