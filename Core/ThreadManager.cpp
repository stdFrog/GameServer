#include "pch.h"
#include "ThreadManager.h"

ThreadManager::ThreadManager() {
	Initialize();
}

ThreadManager::~ThreadManager() {
	Join();
}

void ThreadManager::Launch(std::function<void(void)> Callback) {
	std::lock_guard<std::mutex> Guard(_Lock);

	_Threads.push_back(std::thread([=]() {
		/* 스레드 컨텍스트 */
		Initialize();
		Callback();				// 스레드의 메인 함수
		Destroy();
	}));
}

void ThreadManager::Join() {
	for (std::thread& T : _Threads) {
		if (T.joinable()) {
			T.join();
		}
	}

	_Threads.clear();
}

void ThreadManager::Initialize() {
	static std::atomic<unsigned int> StaticThreadID = 1;
	LocalThreadID = StaticThreadID.fetch_add(1);
}

void ThreadManager::Destroy() {

}