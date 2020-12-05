#pragma once

class FlvStream
{
private:
	FILE* m_hFile;
	uint32_t m_VideoTick;
	uint32_t m_AudioTick;
	uint32_t m_VideoCts;
	uint32_t m_VideoPts;
	uint32_t m_LastVideoTick;

public:
	FlvStream();
	~FlvStream();

	void Init();
	void WriteVideo(const std::vector<uint8_t>& packet, bool keyframe);
	void WriteAudio(const std::vector<uint8_t>& packet);
};

