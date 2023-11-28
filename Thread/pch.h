// pch.h: 미리 컴파일된 헤더 파일입니다.
// 아래 나열된 파일은 한 번만 컴파일되었으며, 향후 빌드에 대한 빌드 성능을 향상합니다.
// 코드 컴파일 및 여러 코드 검색 기능을 포함하여 IntelliSense 성능에도 영향을 미칩니다.
// 그러나 여기에 나열된 파일은 빌드 간 업데이트되는 경우 모두 다시 컴파일됩니다.
// 여기에 자주 업데이트할 파일을 추가하지 마세요. 그러면 성능이 저하됩니다.

#ifndef PCH_H
#define PCH_H

// 여기에 미리 컴파일하려는 헤더 추가
#include "framework.h"
#include <thread>
#include <functional>
#include <mutex>
#include <vector>

#include <stdio.h>
#include<tchar.h>

#include <stdlib.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <mmsystem.h>

#include <Windows.h>
#include <fcntl.h>
#include <io.h>

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"mswsock.lib")
#pragma comment(lib,"Winmm.lib")

#define SOCKET_BUFSIZE (1024*10)
#define PORT 3598

#define CRASH(cause)						\
{											\
	unsigned int* crash = nullptr;				\
	_Analysis_assume_(crash!= nullptr);		\
	*crash = 0xDEADBEEF;					\
}

#define ASSERT_CRASH(expr, err)			\
{									\
	if(!(expr))						\
	{								\
		printf("%s\n",err);							\
		CRASH("ASSERT_CRASH");		\
		_Analysis_assume_(expr);	\
	}								\
}

#endif //PCH_H
