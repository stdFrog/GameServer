#include "pch.h"
#include "GameSession.h"
#include "GameSessionManager.h"

void GameSession::OnConnected() {
	GSessionManager.Add(std::static_pointer_cast<GameSession>(shared_from_this()));
}

void GameSession::OnDisconnected() {
	GSessionManager.Remove(std::static_pointer_cast<GameSession>(shared_from_this()));
}

INT GameSession::OnRecv(PBYTE Buffer, INT Length) {
	std::cout << "OnRecv Length = " << Length << std::endl;

	std::shared_ptr<SendBuffer> NewBuffer = std::make_shared<SendBuffer>(0x1000);
	NewBuffer->CopyData(Buffer, Length);

	GSessionManager.Broadcast(NewBuffer);

	return Length;
}

void GameSession::OnSend(INT Length) {
	std::cout << "OnSend Length = " << Length << std::endl;
}