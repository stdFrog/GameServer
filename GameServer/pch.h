#pragma once

#include "CorePch.h"

#include "Protocol.pb.h"
#include "Enum.pb.h"
#include "Struct.pb.h"

#ifdef _DEBUG
#pragma comment(lib, "Core\\Debug\\Core.lib")
#pragma comment(lib, "Protobuf\\Debug\\libprotobufd.lib")
#else
#pragma comment(lib, "Core\\Release\\Core.lib")
#pragma comment(lib, "Protobuf\\Release\\libprotobuf.lib")
#endif