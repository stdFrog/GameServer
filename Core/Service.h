#pragma once
#include "NetAddress.h"
#include "IOCPCore.h"
#include "Listener.h"
#include <functional>

enum class ServiceType : UCHAR {
	SERVER,
	CLIENT
};

/*
	서비스는 보통 백그라운드에서 실행되는 프로그램을 말한다.
	
	곧, 사용자에게 알릴 필요없는 부가 기능을 담당한다.
	시스템의 유지, 데이터 제공, 하드웨어 관리 등등 여러 가지 일을 한다.

	일반적으로 서비스는 사용자와의 직접적인 상호작용이 필요하지 않다는 것이 특징인데
	여기서 말하는 서비스는 조금 다를 수 있다.
*/
class Service : public std::enable_shared_from_this<Service>
{
};

