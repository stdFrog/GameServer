#pragma once

#include "Types.h"
#include "Macro.h"
#include "TLS.h"
#include "Global.h"

#include <set>
#include <map>
#include <list>
#include <queue>
#include <stack>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include <windows.h>
#include <iostream>

/*
	게임 엔진측에서 사용할 네트워크 관리 도구를 제작한다.

	곧, 서버와 클라이언트에서 모두 사용하게 될 네트워크 도구이며
	서버 프로그래머의 경우 이와 같은 서버 엔진을 제작하여 채팅 프로그램을 목표로 삼으면 된다.

	다른 프로젝트에서도 공통으로 가져야 할 라이브러리 목록이므로 파일을 따로 유지한다.
	pch의 경우 프로젝트 한정적으로만 사용되어 공유 목적으론 사용할 수 없다.
*/