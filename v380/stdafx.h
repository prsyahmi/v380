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
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>
typedef int SOCKET;
#define Sleep sleep
#define memcpy_s(dst, dstsize, src, srcsize)  memcpy(dst, src, srcsize)
#define _stricmp strcasecmp

#endif

#include <stdio.h>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <memory>
#include <vector>
#include <string.h>

