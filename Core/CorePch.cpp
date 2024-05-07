#include "pch.h"
#include "CorePch.h"

/*
	서버 구조에 대해 생각해보자.

	클라이언트 소켓을 어떻게 관리할 것인가, 씬에 등장하는 요소와의 통신, DB 연결 등
	어떻게 해야 깔끔하게 관리할 수 있을지 충분히 고민해야 된다.

	기본적으로 소켓은 자료 구조를 이용해 유지 관리해야만 서로간 통신이 가능하다.
	또, 서버를 몇 개 운영할 것인가에 대해서도 고민해야 한다.

	네트워크와 통신 로직, 스레드 분배(맵마다 서버를 따로 운용/관리) 등이 고려 사항이며,
	소켓을 관리하는 방법은 세션이나 유저 매니저 클래스를 만드는 것이 흔히 쓰인다.

	유저 매니저 클래스는 유저에 대한 정보(계정 정보, IP 등)를 담을 클래스이다.

	브로드캐스팅을 위해 각 객체의 정보가 필요할 수 있다.
	곧, 객체가 보고 있는 뷰포트의 정보가 필요하므로 Actor에 접근할 수 있어야 한다.

	여기까지만 해도 인게임 내에 존재하는 플레이어와 서버에서 만들어둔 소켓간에 주고받을
	데이터가 얼추 정해진다.

	단, 이를 관리하는 방법은 좀 더 고민해봐야 한다.
	매번 플레이어의 일반화된 정보를 주고 받을 필요는 없다.

	따라서, 이 정보를 일시간 저장하여 필요할 때마다 꺼내 쓸 수 있는 관리 클래스가 필요하다.
	곧, 위에서 말한 유저 매니저가 이 역할을 하면 된다.
	다르게 말하면, 모든 소켓을 관리하는 매니저 클래스란 뜻이다.

	매니저 클래스를 이용하기 싫다면, 각 소켓이 어떤 방법으로든 플레이어와 서로 엮여 있어야 한다.
	
	UserManager는 스레드 세이프한가? 아니다. 락 처리 필요
	
	스레드는 클라이언트로부터 전달된 요청에 대하여 거부할 수 있다.
	그러나, 대부분은 요청한 순서대로 처리해야 할 의무가 있다.

	따라서, 각 스레드는 요청받은 작업을 순서대로 저장해둘 필요가 있으며,
	주로 큐라는 자료구조를 이용한다.

	큐는 메인 서버에서 접근 가능해야 하며, 일을 도맡아 처리할 작업 스레드에서도 접근할 수 있어야 한다.
	곧, 메인 서버의 하위 프로세스로써 존재하면 필요할 때 상위 프로세스가 전달한 핸들이나 자료구조의 포인터를 이용해
	접근할 수 있다.

	만약, 씬 리스 게임을 만들고 싶다면 어떻게 해야 할까?
	
	Actor단위로 큐를 배치하는 경우도 있고,
	씬이 전환되는 구역에서 극히 일부 지역에 속한 대상들에 한하여 락을 걸어 통신하는 경우도 있다.

	Actor 단위로 큐를 배치할 경우 스레드가 유닛을 대상으로 매우 빠르게 움직이며 필요한 처리를 한다.
	난이도가 최대치라 생각하면 된다.

	말 그대로 요리사가 고객 한 명 한명 만나서 최상의 음식을 서비스하는 것과 같다.

	구조가 단순할 경우 이런 방법을 활용해도 된다.
	Actor 단위로 큐를 배치한다는 것은, 하나의 유닛이 서버에 요청한 작업을 본인이 가지고 있는 상태를 말한다.
	이 상태에서 작업 스레드는 각각의 유닛이 Update 함수 따위를 통해 전달하는 패킷으로부터
	데이터를 전달받고 필요한 처리를 해낸다.
	
	단, 이러한 구조는 일반화가 어려우므로 쉬워보이진 않는다.

	Actor 단위로 스레드를 배치할 경우 Actor간의 상호작용이 어렵다.

	반면, 큰 구역 단위로 스레드를 배치하면 해당 구역에 속한 모든 통신을
	혼자 처리해야 하므로 수용 가능한 최대 크기가 얼마일지 알 수 없다는 것이다.

	큐를 이용해 작업을 관리하는 경우 락을 거는 시간이 극히 짧아지므로
	최선의 속도를 낼 수 있다.

	곧, 어떤 스레드가 작업을 하건 똑같은 동작을 할 것이 분명하므로 큐에서 작업을 빼오기만 하면 된다.

	서버와 클라이언트 모두 큐에 접근할 방법이 제공되어야 한다.
*/