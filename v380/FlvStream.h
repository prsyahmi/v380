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
	bool m_EnableVideo;
	bool m_EnableAudio;

public:
	FlvStream();
	~FlvStream();

	void Init(bool enableVideo, bool enableAudio);
	void WriteVideo(const std::vector<uint8_t>& packet, bool keyframe);
	void WriteAudio(const std::vector<uint8_t>& packet);
};

