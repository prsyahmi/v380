// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifdef _WIN32

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <io.h>
#include <tchar.h>

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

typedef int SOCKET;
#define Sleep sleep
#define memcpy_s(dst, dstsize, src, srcsize)  memcpy(dst, src, srcsize)
#define _stricmp strcasecmp

static int WSAGetLastError()
{
	return errno;
}

#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#define WSAEWOULDBLOCK EWOULDBLOCK
#define closesocket(x)  close(x)

#endif

#include <stdio.h>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <memory>
#include <vector>
#include <string.h>
#include <chrono>
#include <algorithm>
#include <iterator>
#include <thread>
#include <mutex>
#include <deque>
#include <condition_variable>

#include "UtlSemaphore.h"
