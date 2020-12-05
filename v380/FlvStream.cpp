#include "stdafx.h"
#include "FlvStream.h"

// https://mradionov.github.io/h264-bitstream-viewer/

#ifdef _WIN32
#define bswap_u16 _byteswap_ushort
#define bswap_u32 _byteswap_ulong
#define bswap_u64 _byteswap_uint64
#else
#define bswap_u16 __builtin_bswap16
#define bswap_u32 __builtin_bswap32
#define bswap_u64 __builtin_bswap64
#endif

#pragma pack (push, 1)
struct uint24_t {
	uint8_t a;
	uint8_t b;
	uint8_t c;

	void setSwap(uint32_t n)
	{
		a = (n >> 16) & 0xff;
		b = (n >> 8) & 0xff;
		c = n & 0xff;
	}

	uint32_t getSwap()
	{
		return (a << 16) | (b << 8) | (c);
	}
};

struct int24_t {
	uint8_t a;
	uint8_t b;
	uint8_t c;

	void setSwap(int32_t n)
	{
		a = (n >> 16) & 0xff;
		b = (n >> 8) & 0xff;
		c = n & 0xff;
	}

	int32_t getSwap()
	{
		return (int32_t)((a << 16) | (b << 8) | (c));
	}
};

struct TFlvHeaderTypeFlags
{
	uint8_t TypeFlagsReserved : 5;
	uint8_t TypeFlagsAudio : 1;
	uint8_t TypeFlagsReserved2 : 1;
	uint8_t TypeFlagsVideo : 1;
};
struct TFlvHeader
{
	uint8_t Signature[3]; // FLV
	uint8_t Version;
	uint8_t TypeFlagsVideo : 1;
	uint8_t TypeFlagsReserved2 : 1;
	uint8_t TypeFlagsAudio : 1;
	uint8_t TypeFlagsReserved : 5;
	uint32_t DataOffset;
};

// Layout ---
// TFlvHeader
// FlvBody:
//   PreviousTagSize: Always 0
//   TFlvTag
//   PreviousTagSize: Size of previous tag, including its header. For FLV version 1, this value is 11 plus the DataSize of the previous tag.
//   TFlvTag
//   PreviousTagSize
//   ...

struct TFlvTag
{
	uint8_t TagType;
	uint24_t DataSize;
	uint24_t Timestamp;
	uint8_t TimestampExtended;
	uint24_t StreamID; // Always 0
	// Followed by TFlvAudioData | TFlvVideoData
};

struct TFlvAudioData
{
	uint8_t SoundType : 1;
	uint8_t SoundSize : 1;
	uint8_t SoundRate : 2;
	uint8_t SoundFormat : 4;
	// Followed by SoundData
};

struct TFlvAacAudioData
{
	uint8_t AACPacketType; // 0 = sequence header, 1 = raw data
	// Followed by SoundData
};

struct TFlvVideoData
{
	uint8_t CodecID : 4;
	uint8_t FrameType : 4;
	// Followed by VideoData
};

struct TFlvAvcVideoPacket
{
	uint8_t AVCPacketType;
	int24_t CompositionTime;
	// Followed by VideoData
};

struct TNalHeader
{
	uint8_t nal_unit_type : 5;
	uint8_t nal_ref_idc : 2;
	uint8_t forbidden_zero_bit : 1;
};

struct TParameterSets
{
	uint16_t parameterSetLength;
	// uint8_t * parameterSetLength = Data...
};

struct TAVCDecoderConfigurationRecord
{
	uint8_t configurationVersion; // 1
	uint8_t AVCProfileIndication;
	uint8_t profile_compatibility;
	uint8_t AVCLevelIndication;
	// bit(6) reserved = '111111'b;
	uint8_t lengthSizeMinusOne : 2;
	uint8_t reserved : 6;
	uint8_t numOfSequenceParameterSets : 5;
	uint8_t reserved2 : 3;
	// TParameterSets sps;
	// uint8_t numOfPictureParameterSets;
	// TParameterSets pps;
};

#pragma pack (pop)

uint8_t onMetadata[304] = {
	0x12, 0x00, 0x01, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x0A, 0x6F, 0x6E,
	0x4D, 0x65, 0x74, 0x61, 0x44, 0x61, 0x74, 0x61, 0x08, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x08, 0x64,
	0x75, 0x72, 0x61, 0x74, 0x69, 0x6F, 0x6E, 0x00, 0x40, 0x4F, 0x4A, 0xC0, 0x83, 0x12, 0x6E, 0x98,
	0x00, 0x05, 0x77, 0x69, 0x64, 0x74, 0x68, 0x00, 0x40, 0x9E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x06, 0x68, 0x65, 0x69, 0x67, 0x68, 0x74, 0x00, 0x40, 0x90, 0xE0, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x0D, 0x76, 0x69, 0x64, 0x65, 0x6F, 0x64, 0x61, 0x74, 0x61, 0x72, 0x61, 0x74, 0x65,
	0x00, 0x40, 0xA3, 0x12, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x66, 0x72, 0x61, 0x6D, 0x65,
	0x72, 0x61, 0x74, 0x65, 0x00, 0x40, 0x4E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x76,
	0x69, 0x64, 0x65, 0x6F, 0x63, 0x6F, 0x64, 0x65, 0x63, 0x69, 0x64, 0x00, 0x40, 0x1C, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x61, 0x75, 0x64, 0x69, 0x6F, 0x64, 0x61, 0x74, 0x61, 0x72,
	0x61, 0x74, 0x65, 0x00, 0x40, 0x5F, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x61, 0x75,
	0x64, 0x69, 0x6F, 0x73, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x72, 0x61, 0x74, 0x65, 0x00, 0x40, 0xE5,
	0x88, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x61, 0x75, 0x64, 0x69, 0x6F, 0x73, 0x61, 0x6D,
	0x70, 0x6C, 0x65, 0x73, 0x69, 0x7A, 0x65, 0x00, 0x40, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x06, 0x73, 0x74, 0x65, 0x72, 0x65, 0x6F, 0x01, 0x01, 0x00, 0x0C, 0x61, 0x75, 0x64, 0x69,
	0x6F, 0x63, 0x6F, 0x64, 0x65, 0x63, 0x69, 0x64, 0x00, 0x40, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x07, 0x65, 0x6E, 0x63, 0x6F, 0x64, 0x65, 0x72, 0x02, 0x00, 0x0D, 0x4C, 0x61, 0x76,
	0x66, 0x35, 0x37, 0x2E, 0x38, 0x34, 0x2E, 0x31, 0x30, 0x30, 0x00, 0x08, 0x66, 0x69, 0x6C, 0x65,
	0x73, 0x69, 0x7A, 0x65, 0x00, 0x41, 0xAD, 0x18, 0x2B, 0x2E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09
};

const uint32_t offsetDuration = 40;
const uint32_t offsetWidth = 56;
const uint32_t offsetHeight = 73;
const uint32_t offsetVideoDataRate = 97;
const uint32_t offsetFramerate = 117;
const uint32_t offsetVideoCodecId = 140;
const uint32_t offsetAudioDataRate = 164;
const uint32_t offsetAudioSampleRate = 190;
const uint32_t offsetAudioSampleSize = 216;
const uint32_t offsetAudioStereo = 233; //u8
const uint32_t offsetAudioCodecId = 249;
const uint32_t offsetFilesize = 293;


const uint16_t ADAPTATION_TABLE[] = {
	230, 230, 230, 230, 307, 409, 512, 614,
		768, 614, 512, 409, 307, 230, 230, 230,
};

void WriteDouble(uint8_t* offset, double value)
{
	uint64_t n = *(uint64_t*)&value;
	*(uint64_t*)offset = bswap_u64(n);
}

FlvStream::FlvStream()
	: m_AudioTick(0)
	, m_VideoDts(0)
	, m_VideoPts(0)
	, m_hFile(0)
	, m_LastVideoTick(0)
	, m_EnableVideo(false)
	, m_EnableAudio(false)
	, m_Exit(false)
{
	m_Thread = std::thread([&]() {
		std::deque<TQueue> packetQueue;

		while (!m_Exit) {
			{
				//std::lock_guard<std::mutex> lg(m_Mutex);
				m_Semaphore.wait();
				packetQueue.swap(m_PacketQueue);
			}

			while (packetQueue.size()) {
				const TQueue& q = packetQueue.front();
				if (q.m_Type == eQueueType_Video) {
					WriteVideo(q.m_Packet, false);
				} else if (q.m_Type == eQueueType_VideoKeyFrame) {
					WriteVideo(q.m_Packet, true);
				} else if (q.m_Type == eQueueType_Audio) {
					WriteAudio(q.m_Packet);
				}

				packetQueue.pop_front();
			}
		}
	});
}


FlvStream::~FlvStream()
{
	if (m_hFile) {
		fclose(m_hFile);
	}

	m_Exit = true;
	m_Semaphore.notify();
	if (m_Thread.joinable()) m_Thread.join();
}

void FlvStream::Init(bool enableVideo, bool enableAudio)
{
	TFlvHeader header;

	m_EnableVideo = enableVideo;
	m_EnableAudio = enableAudio;

	header.Signature[0] = 'F';
	header.Signature[1] = 'L';
	header.Signature[2] = 'V';
	header.Version = 1;
	header.TypeFlagsReserved = 0;
	header.TypeFlagsAudio = enableAudio ? 1 : 0;
	header.TypeFlagsReserved2 = 0;
	header.TypeFlagsVideo = m_EnableVideo ? 1 : 0;
	header.DataOffset = bswap_u32(sizeof(TFlvHeader));

	fwrite(&header, 1, sizeof(TFlvHeader), stdout);

	uint32_t prevTagSize = 0;
	fwrite(&prevTagSize, 1, sizeof(uint32_t), stdout);

	std::vector<uint8_t> vMetadata;
	vMetadata.resize(sizeof(onMetadata));
	memcpy_s(vMetadata.data(), vMetadata.size(), onMetadata, sizeof(onMetadata));

	WriteDouble(vMetadata.data() + offsetFramerate, 20);
	WriteDouble(vMetadata.data() + offsetVideoCodecId, 7);
	WriteDouble(vMetadata.data() + offsetAudioSampleRate, 8000);
	WriteDouble(vMetadata.data() + offsetAudioSampleSize, 8000);
	WriteDouble(vMetadata.data() + offsetAudioCodecId, 1);
	WriteDouble(vMetadata.data() + offsetDuration, 0);
	fwrite(vMetadata.data(), 1, vMetadata.size(), stdout);

	prevTagSize = bswap_u32(vMetadata.size());
	fwrite(&prevTagSize, 1, sizeof(uint32_t), stdout);
}

void FlvStream::WriteVideo(const std::vector<uint8_t>& packet, bool keyframe)
{
	if (!m_EnableVideo || m_Exit) {
		return;
	}

	if (m_Thread.get_id() != std::this_thread::get_id()) {
		m_PacketQueue.push_back({
			(keyframe ? eQueueType_VideoKeyFrame : eQueueType_Video),
			packet
		});
		m_Semaphore.notify();
		return;
	}

	TFlvTag tag;
	TFlvVideoData vidData;
	TFlvAvcVideoPacket avc;

	std::vector<std::vector<uint8_t>> Nals;
	std::vector<std::vector<uint8_t>> SPSs;
	std::vector<std::vector<uint8_t>> PPSs;
	uint32_t paramSetsSize = 0;
	std::vector<uint8_t> StartBytes;

	uint32_t rdts = *(uint32_t*)(packet.data() + 0);
	uint32_t rpts = *(uint32_t*)(packet.data() + 8);
	uint32_t unk0 = *(uint32_t*)(packet.data() + 4);
	uint32_t unk1 = *(uint32_t*)(packet.data() + 12);
	uint32_t unk2 = *(uint32_t*)(packet.data() + 16);
	uint32_t unk3 = *(uint32_t*)(packet.data() + 20);

	uint32_t dts = m_VideoDts ? rdts - m_VideoDts : 0;
	uint32_t pts = m_VideoPts ? rpts - m_VideoPts : 0;
	if (m_VideoDts == 0) {
		m_VideoDts = rdts;
		m_VideoPts = rpts;
	}

	uint32_t timestamp = pts;
	uint32_t cts = 0;

	//fprintf(stderr, "dts=%08d, pts=%08d, ts=%08d, cts=%08d | unk0=%08d unk1=%08d unk2=%08X size=%08d\n",
	//	dts, pts, timestamp, cts, unk0, unk1, unk2, packet.size());

	StartBytes.resize(4);
	StartBytes[3] = 1;

	bool endOfSearch = false;
	size_t prevOffset = 0;
	size_t totalNalSize = 0;
	do {
		auto it = std::search(
			std::begin(packet) + (prevOffset ? prevOffset : 0), std::end(packet),
			std::begin(StartBytes), std::end(StartBytes));

		endOfSearch = it == packet.end();
		size_t curOffset = endOfSearch ? packet.size() : std::distance(std::begin(packet), it);
		if (prevOffset) {
			// Create NAL unit from byte stream with boundary
			// Consists of:
			//    unit size (uint32_t)
			//    NAL headers
			//    data
			std::vector<uint8_t> nal;
			uint32_t nalSize = bswap_u32(curOffset - prevOffset);
			TNalHeader* nalHeader = (TNalHeader*)(packet.data() + prevOffset);

			if (nalHeader->forbidden_zero_bit == 0) {
				if (nalHeader->nal_unit_type == 7) {
					// SPS
					nal.insert(nal.end(), packet.begin() + prevOffset, packet.begin() + curOffset);
					SPSs.push_back(nal);
					paramSetsSize += nal.size();
				} else if (nalHeader->nal_unit_type == 8) {
					// PPS
					nal.insert(nal.end(), packet.begin() + prevOffset, packet.begin() + curOffset);
					PPSs.push_back(nal);
					paramSetsSize += nal.size();
				} else {
					nal.resize(4);
					*(uint32_t*)nal.data() = nalSize;
					nal.insert(nal.end(), packet.begin() + prevOffset, packet.begin() + curOffset);

					Nals.push_back(nal);
					totalNalSize += nal.size();
				}
			}
		}

		prevOffset = curOffset + 4; // Not Include boundary
		//prevOffset = curOffset; // Include boundary
	} while (!endOfSearch);

	if (SPSs.size() || PPSs.size())
	{
		// Write AVCDecoderConfigurationRecord first
		std::vector<uint8_t> AvcDcrData;
		TAVCDecoderConfigurationRecord* dcr;

		AvcDcrData.resize(sizeof(TAVCDecoderConfigurationRecord) + (SPSs.size() * sizeof(uint16_t) + 8 + PPSs.size() * sizeof(uint16_t) + paramSetsSize));
		uint8_t* pDcr = (uint8_t*)(AvcDcrData.data() + sizeof(TAVCDecoderConfigurationRecord));

		dcr = (TAVCDecoderConfigurationRecord*)AvcDcrData.data();
		dcr->configurationVersion = 1;
		dcr->AVCProfileIndication = 77;
		dcr->profile_compatibility = 0;
		dcr->AVCLevelIndication = 15;
		dcr->lengthSizeMinusOne = 3;
		dcr->numOfSequenceParameterSets = SPSs.size();
		for (auto sps : SPSs) {
			*(uint16_t*)pDcr = bswap_u16((uint16_t)sps.size());
			pDcr += sizeof(uint16_t);
			memcpy_s(pDcr, sps.size(), sps.data(), sps.size());
			pDcr += sps.size();
		}
		*(uint8_t*)pDcr = (uint8_t)PPSs.size();
		pDcr += sizeof(uint8_t);
		for (auto pps : PPSs) {
			*(uint16_t*)pDcr = bswap_u16((uint16_t)pps.size());
			pDcr += sizeof(uint16_t);
			memcpy_s(pDcr, pps.size(), pps.data(), pps.size());
			pDcr += pps.size();
		}

		tag.TagType = 9; // video;
		tag.DataSize.setSwap(sizeof(vidData) + sizeof(avc) + AvcDcrData.size());
		tag.Timestamp.setSwap(0);
		tag.TimestampExtended = 0;
		tag.StreamID.setSwap(0);
		fwrite(&tag, 1, sizeof(tag), stdout);
		vidData.FrameType = 1;
		vidData.CodecID = 7;
		fwrite(&vidData, 1, sizeof(vidData), stdout);
		avc.AVCPacketType = 0;
		avc.CompositionTime.setSwap(0);
		fwrite(&avc, 1, sizeof(avc), stdout);
		fwrite(AvcDcrData.data(), 1, AvcDcrData.size(), stdout);

		uint32_t prevTagSize = bswap_u32(sizeof(tag) + sizeof(vidData) + sizeof(avc) + AvcDcrData.size());
		fwrite(&prevTagSize, 1, sizeof(uint32_t), stdout);
	}

	// Not sure yet how to construct timestamp and compositionTime
	// https://stackoverflow.com/questions/7054954/the-composition-timects-when-wrapping-h-264-nalus

	tag.TagType = 9; // video;
	tag.DataSize.setSwap(sizeof(vidData) + sizeof(avc) + totalNalSize);
	tag.Timestamp.setSwap(timestamp);
	tag.TimestampExtended = 0;
	tag.StreamID.setSwap(0);
	fwrite(&tag, 1, sizeof(tag), stdout);

	vidData.FrameType = keyframe ? 1 : 2;
	vidData.CodecID = 7;
	fwrite(&vidData, 1, sizeof(vidData), stdout);

	avc.AVCPacketType = 1;
	avc.CompositionTime.setSwap(cts);
	fwrite(&avc, 1, sizeof(avc), stdout);

	for (auto it = Nals.begin(); it != Nals.end(); it++) {
		fwrite(it->data(), 1, it->size(), stdout);
	}

	uint32_t prevTagSize = bswap_u32(sizeof(tag) + sizeof(vidData) + sizeof(avc) + totalNalSize);
	fwrite(&prevTagSize, 1, sizeof(uint32_t), stdout);

	fflush(stdout);
}


int adpcm_decoder(int a1, char *a2, int16_t *a3, int a4, int a5);

void FlvStream::WriteAudio(const std::vector<uint8_t>& packet)
{
	if (!m_EnableAudio || m_Exit) {
		return;
	}

	if (m_Thread.get_id() != std::this_thread::get_id()) {
		m_PacketQueue.push_back({
			eQueueType_Audio,
			packet
		});
		m_Semaphore.notify();
		return;
	}

	TFlvTag tag;
	TFlvAudioData audData;
	std::vector<uint8_t> packetOnly(packet.begin() + 20, packet.end());
	std::vector<int16_t> pcmData;

	pcmData.resize(1024);

	// There is no output format for ADPCM 8000hz, we need to convert it internally
	int nPcmData = adpcm_decoder(0, (char *)packetOnly.data(), pcmData.data(), 505, 1);
	uint32_t frameN = *(uint32_t*)(packet.data() + 8);

	tag.TagType = 8; // audio;
	tag.DataSize.setSwap(sizeof(audData) + nPcmData);
	tag.Timestamp.setSwap(m_AudioTick ? GetTickCount() - m_AudioTick : 0);
	tag.TimestampExtended = 0;
	tag.StreamID.setSwap(0);
	fwrite(&tag, 1, sizeof(TFlvTag), stdout);

	// Format 3: linear PCM, stores raw PCM samples.
	// If the data is 8 - bit, the samples are unsigned bytes.
	// If the data is 16 - bit, the samples are stored as little endian, signed numbers.
	// If the data is stereo, left and right samples are stored interleaved : left - right - left - right - and so on.
	audData.SoundFormat = 3; 
	audData.SoundRate = 1;
	audData.SoundSize = 1;
	audData.SoundType = 0;
	fwrite(&audData, 1, sizeof(audData), stdout);

	fwrite(pcmData.data(), 1, nPcmData, stdout);

	uint32_t prevTagSize = bswap_u32(sizeof(tag) + sizeof (audData) + nPcmData);
	fwrite(&prevTagSize, 1, sizeof(uint32_t), stdout);

	if (m_AudioTick == 0) {
		m_AudioTick = GetTickCount();
	}

	fflush(stdout);
}

const uint32_t asc_E5F0[] = {
	0x07, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00,
	0x0B, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00,
	0x10, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00,
	0x17, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00,
	0x22, 0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x2D, 0x00, 0x00, 0x00,
	0x32, 0x00, 0x00, 0x00, 0x37, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00,
	0x49, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x58, 0x00, 0x00, 0x00, 0x61, 0x00, 0x00, 0x00,
	0x6B, 0x00, 0x00, 0x00, 0x76, 0x00, 0x00, 0x00, 0x82, 0x00, 0x00, 0x00, 0x8F, 0x00, 0x00, 0x00,
	0x9D, 0x00, 0x00, 0x00, 0xAD, 0x00, 0x00, 0x00, 0xBE, 0x00, 0x00, 0x00, 0xD1, 0x00, 0x00, 0x00,
	0xE6, 0x00, 0x00, 0x00, 0xFD, 0x00, 0x00, 0x00, 0x17, 0x01, 0x00, 0x00, 0x33, 0x01, 0x00, 0x00,
	0x51, 0x01, 0x00, 0x00, 0x73, 0x01, 0x00, 0x00, 0x98, 0x01, 0x00, 0x00, 0xC1, 0x01, 0x00, 0x00,
	0xEE, 0x01, 0x00, 0x00, 0x20, 0x02, 0x00, 0x00, 0x56, 0x02, 0x00, 0x00, 0x92, 0x02, 0x00, 0x00,
	0xD4, 0x02, 0x00, 0x00, 0x1C, 0x03, 0x00, 0x00, 0x6C, 0x03, 0x00, 0x00, 0xC3, 0x03, 0x00, 0x00,
	0x24, 0x04, 0x00, 0x00, 0x8E, 0x04, 0x00, 0x00, 0x02, 0x05, 0x00, 0x00, 0x83, 0x05, 0x00, 0x00,
	0x10, 0x06, 0x00, 0x00, 0xAB, 0x06, 0x00, 0x00, 0x56, 0x07, 0x00, 0x00, 0x12, 0x08, 0x00, 0x00,
	0xE0, 0x08, 0x00, 0x00, 0xC3, 0x09, 0x00, 0x00, 0xBD, 0x0A, 0x00, 0x00, 0xD0, 0x0B, 0x00, 0x00
};

const uint32_t dword_E6F0[] = {
	0xFF, 0x0C, 0x00, 0x00, 0x4C, 0x0E, 0x00, 0x00, 0xBA, 0x0F, 0x00, 0x00, 0x4C, 0x11, 0x00, 0x00,
	0x07, 0x13, 0x00, 0x00, 0xEE, 0x14, 0x00, 0x00, 0x06, 0x17, 0x00, 0x00, 0x54, 0x19, 0x00, 0x00,
	0xDC, 0x1B, 0x00, 0x00, 0xA5, 0x1E, 0x00, 0x00, 0xB6, 0x21, 0x00, 0x00, 0x15, 0x25, 0x00, 0x00,
	0xCA, 0x28, 0x00, 0x00, 0xDF, 0x2C, 0x00, 0x00, 0x5B, 0x31, 0x00, 0x00, 0x4B, 0x36, 0x00, 0x00,
	0xB9, 0x3B, 0x00, 0x00, 0xB2, 0x41, 0x00, 0x00, 0x44, 0x48, 0x00, 0x00, 0x7E, 0x4F, 0x00, 0x00,
	0x71, 0x57, 0x00, 0x00, 0x2F, 0x60, 0x00, 0x00, 0xCE, 0x69, 0x00, 0x00, 0x62, 0x74, 0x00, 0x00,
	0xFF, 0x7F, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00
};

int alaw_compress(int16_t a1)
{
	int16_t v1; // r3
	int16_t v2; // r3
	int16_t v3; // r1
	int16_t v4; // r3
	int16_t v5; // r2
	int v6; // r2
	unsigned __int8 v8; // [sp+6h] [bp-2h]

	v1 = a1 >> 2;
	if (a1 < 0)
		v1 = ~a1 >> 2;
	v2 = (v1 + 33) & 0xFFFF;
	if (v2 > 0x1FFF)
		v2 = 0x1FFF;
	v3 = v2;
	v4 = ((signed __int16)v2 >> 6) & 0xFFFF;
	v5 = 1;
	while (v4)
	{
		v5 = (v5 + 1) & 0xFFFF;
		v4 = ((signed __int16)v4 >> 1) & 0xFFFF;
	}
	v6 = (~(v3 >> v5) & 0xF | 16 * (signed __int16)(8 - v5)) & 0xFFFF;
	v8 = v6;
	if (a1 >= 0)
		v8 = v6 | 0x80;
	return v8;
}

int adpcm_decoder(int a1, char *a2, int16_t *a3, int a4, int a5)
{
	signed int v5; // r5
	int16_t *v6; // r6
	int v7; // r4
	int v8; // r0
	int v9; // r0
	int16_t *v10; // r6
	int v11; // r7
	int v12; // r2
	char v13; // r1
	int v14; // r1
	char v15; // r2
	int v16; // r3
	int v17; // r2
	int v18; // r0
	int v19; // r0
	int result; // r0
	int v21; // [sp+0h] [bp-38h]
	char *v22; // [sp+4h] [bp-34h]
	int v23; // [sp+8h] [bp-30h]
	int v24; // [sp+Ch] [bp-2Ch]
	int v25; // [sp+18h] [bp-20h]
	int v26; // [sp+1Ch] [bp-1Ch]

	v26 = a4;
	v25 = a1;
	v5 = a2[2];
	v6 = a3;
	v7 = (a2[1] << 8) | *a2;
	v22 = a2 + 4;
	if (a1)
	{
		v8 = alaw_compress((signed __int16)((a2[1] << 8) | *a2));
		v9 = (v8 | (v8 << 8)) & 0xFFFF;
		*v6 = v9;
		v6[1] = v9;
	}
	else
	{
		*a3 = (a2[1] << 8) | *a2;
		a3[1] = v7;
	}
	v10 = &v6[a5];
	v11 = *(uint32_t *)&asc_E5F0[4 * v5];
	v21 = 0;
	v24 = 0;
	v23 = 0;
	while (1)
	{
		result = v21;
		if (v21 >= v26 - 1)
			break;
		if (v24)
		{
			v12 = (v23 >> 4) & 0xF;
		}
		else
		{
			v13 = *v22++;
			v23 = v13;
			v12 = v13 & 0xF;
		}
		v24 ^= 1u;
		v5 += dword_E6F0[v12 + 25];
		if (v5 < 0)
		{
			v5 = 0;
		}
		else if (v5 > 88)
		{
			v5 = 88;
		}
		v14 = v12 & 8;
		v15 = v12 & 7;
		v16 = v11 >> 3;
		if (v15 & 4)
			v16 += v11;
		if (v15 & 2)
			v16 += v11 >> 1;
		if (v15 & 1)
			v16 += v11 >> 2;
		v17 = v7 + v16;
		if (v14)
			v17 = v7 - v16;
		v7 = v17;

		if (v17 < -32768)
			v7 = -32768;
		if (v7 > 0x7FFF)
			v7 = 0x7FFF;

		v11 = *(uint32_t *)&asc_E5F0[4 * v5];
		if (v25)
		{
			v18 = alaw_compress((signed __int16)v7);
			v19 = (v18 | (v18 << 8)) & 0xFFFF;
			*v10 = v19;
			v10[1] = v19;
		}
		else
		{
			*v10 = v7;
			v10[1] = v7;
		}
		v10 += a5;
		++v21;
	}
	return result;
}
