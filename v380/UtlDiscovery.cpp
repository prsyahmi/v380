#include "stdafx.h"
#include "UtlDiscovery.h"


UtlDiscovery::UtlDiscovery()
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

	struct sockaddr_in addr;

	m_Sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_Sock == INVALID_SOCKET) {
		throw std::runtime_error("Failed to create discovery socket " + std::to_string(WSAGetLastError()));
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(10009);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int res = bind(m_Sock, (sockaddr*)&addr, sizeof(addr));
	if (res == SOCKET_ERROR) {
		throw std::runtime_error("Failed to bind discovery socket " + std::to_string(WSAGetLastError()));
	}

	int broadcast_enabled = 1;
	res = setsockopt(m_Sock, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast_enabled, sizeof(broadcast_enabled));
	if (res == SOCKET_ERROR) {
		throw std::runtime_error("Failed to set socket broadcast " + std::to_string(WSAGetLastError()));
	}

#ifdef _WIN32
	u_long nonblocking_enabled = 1;
	res = ioctlsocket(m_Sock, FIONBIO, &nonblocking_enabled);
	if (res == SOCKET_ERROR) {
		throw std::runtime_error("Failed to change socket to non-blocking " + std::to_string(WSAGetLastError()));
	}
#else
	int flags = fcntl(m_Sock, F_GETFL, 0);
	if (flags == SOCKET_ERROR) {
		throw std::runtime_error("Failed to retrieve socket flags " + std::to_string(WSAGetLastError()));
	}

	flags |= O_NONBLOCK;
	res = fcntl(m_Sock, F_SETFL, flags);
	if (res == SOCKET_ERROR) {
		throw std::runtime_error("Failed to change socket to non-blocking " + std::to_string(WSAGetLastError()));
	}
#endif
}

UtlDiscovery::~UtlDiscovery()
{
	if (m_Sock) {
		closesocket(m_Sock);
	}

#ifdef _WIN32
	WSACleanup();
#endif
}

const std::vector<TDevice>& UtlDiscovery::Discover()
{
	sockaddr_in addr;
	std::string buf;
	socklen_t nAddr = sizeof(addr);
	int res;
	int retry = 5;

	memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(10008);
	addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	const char searchcmd[] = "NVDEVSEARCH^100";

	while (retry) {
		res = sendto(m_Sock, searchcmd, strlen(searchcmd), 0, (const sockaddr*)&addr, nAddr);
		if (res == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAEWOULDBLOCK) {
				continue;
			}
			throw std::runtime_error("Discovery failed to send data " + std::to_string(WSAGetLastError()));
		}

		auto lastTick = std::chrono::steady_clock::now();
		do {
			auto curTick = std::chrono::steady_clock::now();
			std::chrono::duration<double, std::milli> diff = curTick - lastTick;
			if (diff.count() >= 250) break;

			buf.clear();
			buf.resize(255);
			nAddr = sizeof(addr);
			res = recvfrom(m_Sock, (char*)buf.data(), buf.size(), 0, (sockaddr*)&addr, &nAddr);
			if (res == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK) {
				res = 0;
				continue;
			}

			if (res > 0) {
				buf.resize(res);
				Parse(buf);
			}
		} while (res >= 0);

		--retry;
	}

	buf.find("^");

	return m_Devices;
}

void UtlDiscovery::Parse(const std::string& data)
{
	std::vector<std::string> v;

	for (size_t off = 0; off != std::string::npos; )
	{
		size_t last = off;
		off = data.find('^', off);

		v.push_back(std::string(
			data.begin() + last,
			off == std::string::npos ? data.end() : data.begin() + off++
		));
	}

	if (v[0] != "NVDEVRESULT") return;

	for (auto it = m_Devices.begin(); it != m_Devices.end(); ++it)
	{
		if (it->mac == v[2]) return;
	}

	m_Devices.push_back({
		v[2], v[12], v[3]
	});
}
