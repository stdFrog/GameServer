#pragma once

#include <thread>
#include <functional>

class ThreadManager
{
	static void Initialize();
	static void Destroy();

	std::mutex _Lock;
	std::vector<std::thread> _Threads;

public:
	ThreadManager();
	~ThreadManager();

	void Launch(std::function<void(void)> Callback);
	void Join();
};

