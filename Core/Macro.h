#pragma once

/*
	뮤텍스는 일반적으로 커널 수준의 동기화 객체를 말한다.
	여기선 STL이 제공하는 std::mutex를 사용하는데 실제 그 구조를 보면 꽤나 복잡하다.

	먼저, 기반 클래스인 _Mutex_base를 살펴보면 void* 타입 두 쌍을 가지는 크리티컬 섹션 구조체와
	void*, unsigned int 타입을 가지는 스레드 구조체, 횟수와 타입을 지정하는 int형 변수 두 쌍으로 이루어져 있다.

	이 기반 클래스로부터 파생된 클래스가 std::mutex를 구성하는데,
	사실상 timed_mutex, condition_variable, unique_lock 클래스가 주요 구성원이다.

	여기서 주의깊게 살펴봐야 하는 부분은 어떻게 동기화 기법을 구현하는가인데,
	사실상 다중 구조의 while문을 통해 스레드 간의 순서를 정한다.

	timed_mutex는 별도의 래퍼 클래스로, mutex 클래스를 포함하고 있으며 lock_guard 역시 mutex 클래스를 포함한다.

	두 클래스 모두 생성자 또는 멤버 변수로 잠금 기능을 호출하거나 구현하고 있는데,
	상호간 구조적으로 작성되어 있어 전체 흐름을 살펴보기 위해선 따로 파일을 만들어
	순서대로 살펴보는 것이 좋다.

	작성자가 아직 그정도 수준은 아니므로 STL의 동기화 객체에 대해선 추후 시간내어 따로 정리하도록 하자.
*/
#define USE_MANY_LOCKS(Count) std::mutex _Locks[Count];
#define USE_LOCK USE_MANY_LOCKS(1)
#define WRITE_LOCK_INDEX(Index) std::lock_guard<std::mutex> WriteGuard_##Index(_Locks[Index]);
#define WRITE_LOCK WRITE_LOCK_INDEX(0)
/*
	참고로 표준 뮤텍스의 경우 중첩 또는 포기된 뮤텍스를 인정하지 않으므로 충돌 가능성을
	충분히 검토하고 극히 짧은 순간에만 활용할 수 있도록 유의해야 한다.
*/