#include "pch.h"
#include "Global.h"
#include "ThreadManager.h"

/* 메모리의 이동이 막혀있기 때문에 유일 객체로써 활용 가능 */
std::unique_ptr<ThreadManager> GThreadManager = std::make_unique<ThreadManager>();