#pragma once

enum EQueueType {
	eQueueType_Video,
	eQueueType_VideoKeyFrame,
	eQueueType_Audio,
};

struct TQueue
{
	EQueueType m_Type;
	std::vector<uint8_t> m_Packet;
};

class FlvStream
{
private:
	FILE* m_hFile;
	uint32_t m_AudioTick;
	uint32_t m_VideoDts;
	uint32_t m_VideoPts;
	uint32_t m_LastVideoTick;
	bool m_EnableVideo;
	bool m_EnableAudio;

	bool m_Exit;
	std::thread m_Thread;
	std::mutex m_Mutex;
	Semaphore m_Semaphore;
	std::deque<TQueue> m_PacketQueue;

public:
	FlvStream();
	~FlvStream();

	void Init(bool enableVideo, bool enableAudio);
	void WriteVideo(const std::vector<uint8_t>& packet, bool keyframe);
	void WriteAudio(const std::vector<uint8_t>& packet);
};
