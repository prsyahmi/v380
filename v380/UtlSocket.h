#pragma once

class UtlSocket
{
private:
	SOCKET m_Sock;

public:
	UtlSocket();
	~UtlSocket();

	void Connect(const std::string& ip, const std::string& port);
	void Close();

	bool DisableNagle(bool state = true);

	int Send(const void* pubData, size_t length);
	int Send(std::vector<uint8_t>& buffer);

	int Recv(void* out, size_t length, int timeoutMillis = 0);
	int Recv(std::vector<uint8_t>& buffer, int timeoutMillis = 0);
	int Recv(std::vector<uint8_t>& buffer, size_t length, int timeoutMillis = 0);
};

