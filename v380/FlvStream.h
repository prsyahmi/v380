#pragma once

class FlvStream
{
private:
	FILE* m_hFile;
	DWORD m_VideoTick;
	DWORD m_AudioTick;
	DWORD m_VideoCts;
	DWORD m_VideoPts;
	DWORD m_LastVideoTick;
	bool m_WrittenAudio;

public:
	FlvStream();
	~FlvStream();

	void Init();
	void WriteVideo(const std::vector<uint8_t>& packet, bool keyframe);
	void WriteAudio(const std::vector<uint8_t>& packet);
};

