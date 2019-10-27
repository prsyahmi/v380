#include "stdafx.h"
#include "UtlSocket.h"

#pragma comment(lib, "ws2_32.lib")

UtlSocket::UtlSocket()
	: m_Sock(INVALID_SOCKET)
{
#ifdef _WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		throw std::runtime_error("WSAStartup failed with error" + std::to_string(err));
	}
#endif
}

UtlSocket::~UtlSocket()
{
	if (m_Sock) {
		closesocket(m_Sock);
	}

#ifdef _WIN32
	WSACleanup();
#endif
}

void UtlSocket::Connect(const std::string& ip, const std::string& port)
{
	int iResult;
	struct addrinfo *tmpAddrInfo, //*result = NULL,
		*ptr = NULL,
		hints;

	auto addrinfo_deleter = [](struct addrinfo* ptr)->void { freeaddrinfo(ptr); };
	std::unique_ptr<struct addrinfo, decltype(addrinfo_deleter)> result(0, addrinfo_deleter);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port and try each interfaces to connect to this server
	iResult = getaddrinfo(ip.c_str(), port.c_str(), &hints, &tmpAddrInfo);
	if (iResult != 0) {
		throw std::runtime_error("Unable to resolve address at " + ip + ":" + port);
	}

	result.reset(tmpAddrInfo);

	for (ptr = result.get(); ptr != NULL; ptr = ptr->ai_next)
	{
		m_Sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (m_Sock == INVALID_SOCKET) {
			throw std::runtime_error("Failed to create socket " + std::to_string(WSAGetLastError()));
		}

		iResult = connect(m_Sock, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(m_Sock);
			m_Sock = INVALID_SOCKET;
			continue;
		}
	}
}

void UtlSocket::Close()
{
	if (m_Sock) {
		closesocket(m_Sock);
	}

	m_Sock = INVALID_SOCKET;
}

bool UtlSocket::DisableNagle(bool state)
{
	int flag = state ? 1 : 0;
	int result = setsockopt(m_Sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
	
	return result == 0;
}

int UtlSocket::Send(const void * pubData, size_t length)
{
	int res = send(m_Sock, (const char*)pubData, (int)length, 0);
	if (res == SOCKET_ERROR) {
		throw std::runtime_error("Send failed with error " + std::to_string(WSAGetLastError()));
	}

	return res;
}

int UtlSocket::Send(std::vector<uint8_t>& buffer)
{
	return Send(buffer.data(), buffer.size());
}

int UtlSocket::Recv(void* out, size_t length)
{
	memset(out, 0, length);
	int res = recv(m_Sock, (char*)out, (int)length, 0);

	if (res == 0)
	{
		throw std::runtime_error("Connection closed");
	}
	else if (res < 0)
	{
		throw std::runtime_error("Recieve failed with error " + std::to_string(WSAGetLastError()));
	}

	return res;
}

int UtlSocket::Recv(std::vector<uint8_t>& buffer)
{
	return Recv(buffer.data(), buffer.size());
}

int UtlSocket::Recv(std::vector<uint8_t>& buffer, size_t length)
{
	buffer.resize(length);
	return Recv(buffer.data(), buffer.size());
}
