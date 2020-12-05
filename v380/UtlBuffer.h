#pragma once

class UtlBuffer
{
private:
	std::vector<uint8_t> m_Buff;
	uint32_t m_Offset;

public:
	UtlBuffer()
		: m_Offset(0)
	{}

	std::vector<uint8_t>& GetBuffer()
	{
		return m_Buff;
	}

	uint32_t Write(const void* pubData, uint32_t cubData)
	{
		m_Buff.resize(m_Buff.size() + cubData);
		memcpy_s(m_Buff.data() + m_Offset, cubData, pubData, cubData);
		m_Offset += cubData;

		return cubData;
	}

	template<class T>
	uint32_t Write(const std::vector<T>& data)
	{
		return Write(data.data(), data.size());
	}

	template<class T>
	uint32_t Write(const T& data)
	{
		return Write(&data, sizeof(T));
	}
};
