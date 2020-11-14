#include "stdafx.h"
#include "FlvStream.h"

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

void WriteDouble(uint8_t* offset, double value)
{
	uint64_t n = *(uint64_t*)&value;
	*(uint64_t*)offset = bswap_u64(n);
}

FlvStream::FlvStream()
	: m_VideoTick(0)
	, m_AudioTick(0)
	, m_VideoCts(0)
	, m_VideoPts(0)
	, m_hFile(0)
{
}


FlvStream::~FlvStream()
{
	if (m_hFile) {
		fclose(m_hFile);
	}
}

void FlvStream::Init()
{
	TFlvHeader header;
	header.Signature[0] = 'F';
	header.Signature[1] = 'L';
	header.Signature[2] = 'V';
	header.Version = 1;
	header.TypeFlagsReserved = 0;
	header.TypeFlagsAudio = 1;
	header.TypeFlagsReserved2 = 0;
	header.TypeFlagsVideo = 1;
	header.DataOffset = bswap_u32(sizeof(TFlvHeader));

	fwrite(&header, 1, sizeof(TFlvHeader), stdout);

	uint32_t prevTagSize = 0;
	fwrite(&prevTagSize, 1, sizeof(uint32_t), stdout);

	/*std::vector<uint8_t> vMetadata;
	vMetadata.resize(sizeof(onMetadata));
	memcpy_s(vMetadata.data(), vMetadata.size(), onMetadata, sizeof(onMetadata));

	WriteDouble(vMetadata.data() + offsetFramerate, 20);
	WriteDouble(vMetadata.data() + offsetVideoCodecId, 7);
	WriteDouble(vMetadata.data() + offsetAudioSampleRate, 8000);
	WriteDouble(vMetadata.data() + offsetAudioSampleSize, 8000);
	WriteDouble(vMetadata.data() + offsetAudioCodecId, 1);
	fwrite(vMetadata.data(), 1, vMetadata.size(), stdout);

	prevTagSize = bswap_u32(vMetadata.size());
	fwrite(&prevTagSize, 1, sizeof(uint32_t), stdout);*/
}

void FlvStream::WriteVideo(const std::vector<uint8_t>& packet, bool keyframe)
{
	//if (m_hFile == NULL) {
	//	return;
	//}

	TFlvTag tag;
	TFlvVideoData vidData;
	TFlvAvcVideoPacket avc;

	std::vector<std::vector<uint8_t>> Nals;
	std::vector<std::vector<uint8_t>> SPSs;
	std::vector<std::vector<uint8_t>> PPSs;
	uint32_t paramSetsSize = 0;
	std::vector<uint8_t> StartBytes;

	uint32_t frameN = *(uint32_t*)(packet.data() + 8);
	if (m_VideoTick == 0) {
		m_VideoCts = frameN;
	}

	uint32_t timestamp = (m_VideoTick ? GetTickCount() - m_VideoTick : 0) + (frameN - m_VideoCts);// -(m_VideoPts ? GetTickCount() - m_VideoPts : 0);

	timestamp = m_VideoTick ? (frameN - m_VideoCts) / 2 : 0;

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
	avc.CompositionTime.setSwap(timestamp);
	fwrite(&avc, 1, sizeof(avc), stdout);

	for (auto it = Nals.begin(); it != Nals.end(); it++) {
		fwrite(it->data(), 1, it->size(), stdout);
	}

	uint32_t prevTagSize = bswap_u32(sizeof(tag) + sizeof(vidData) + sizeof(avc) + totalNalSize);
	fwrite(&prevTagSize, 1, sizeof(uint32_t), stdout);

	if (m_VideoTick == 0) {
		m_VideoTick = GetTickCount();
	}

	m_VideoPts = GetTickCount();

	fflush(stdout);
}

void FlvStream::WriteAudio(const std::vector<uint8_t>& packet)
{
	//if (m_hFile == NULL) {
	//	return;
	//}

	TFlvTag tag;
	TFlvAudioData audData;
	std::vector<uint8_t> packetOnly(packet.begin() + 20, packet.end());

	// There is no output format for ADPCM 8000hz, we need to convert it internally

	uint32_t frameN = *(uint32_t*)(packet.data() + 8);
	/*if (m_AudioTick == 0) {
		m_AudioTick = frameN;
	}*/

	tag.TagType = 8; // audio;
	tag.DataSize.setSwap(sizeof(audData) + packetOnly.size());
	tag.Timestamp.setSwap(m_AudioTick ? GetTickCount() - m_AudioTick : 0);
	tag.TimestampExtended = 0;
	tag.StreamID.setSwap(0);
	fwrite(&tag, 1, sizeof(TFlvTag), stdout);

	audData.SoundFormat = 1;
	audData.SoundRate = 0;
	audData.SoundSize = 0;
	audData.SoundType = 0;
	fwrite(&audData, 1, sizeof(audData), stdout);

	fwrite(packetOnly.data(), 1, packetOnly.size(), stdout);

	uint32_t prevTagSize = bswap_u32(sizeof(tag) + sizeof (audData) + packetOnly.size());
	fwrite(&prevTagSize, 1, sizeof(uint32_t), stdout);

	if (m_AudioTick == 0) {
		m_AudioTick = GetTickCount();
	}

	fflush(stdout);
}
