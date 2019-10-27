#pragma once

struct TDevice
{
	std::string mac;
	std::string devid;
	std::string ip;
};

class UtlDiscovery
{
private:
	SOCKET m_Sock;
	std::vector<TDevice> m_Devices;

public:
	UtlDiscovery();
	~UtlDiscovery();

	const std::vector<TDevice>& Discover();
	
private:
	void Parse(const std::string& data);
};
