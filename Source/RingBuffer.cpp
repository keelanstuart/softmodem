#include "pch.h"
#include "RingBuffer.h"


inline size_t BytesWaiting(size_t tail, size_t head, size_t capacity)
{
	return (head + capacity - tail) % capacity - 1;
}


inline size_t SpaceAvailable(size_t tail, size_t head, size_t capacity)
{
	return capacity - 1 - BytesWaiting(tail, head, capacity);
}


RingBuffer::RingBuffer(size_t sz, Callback available, Callback full)
{
	m_Buffer.resize(sz);
	m_Available = available;
	m_Full = full;
	Clear();
}


size_t RingBuffer::Write(const uint8_t *buf, size_t buflen)
{
	if (!buflen)
		return 0;

	if (m_Full && (SpaceAvailable(m_Tail, m_Head, m_Buffer.size()) < buflen))
		m_Full();

	size_t ret = std::min<size_t>(buflen, SpaceAvailable(m_Tail, m_Head, m_Buffer.size()));
	if (!ret)
		return ret;

	if (m_Head < m_Tail)
	{
		memcpy(m_Buffer.data() + m_Head, buf, ret);
	}
	else
	{
		size_t part = std::min<size_t>(ret, m_Buffer.size() - m_Head);
		memcpy(m_Buffer.data() + m_Head, buf, part);
		if (part < ret)
			memcpy(m_Buffer.data(), buf + part, ret - part);
	}

	m_Head = (m_Head + ret) % m_Buffer.size();
	return ret;
}

size_t RingBuffer::Read(uint8_t *buf, size_t buflen)
{
	size_t ret = std::min<size_t>(buflen, BytesWaiting(m_Tail, m_Head, m_Buffer.size()));
	if (!ret)
		return ret;

	if (m_Tail < m_Head)
	{
		memcpy(buf, m_Buffer.data() + m_Tail, ret);
	}
	else
	{
		size_t part = std::min<size_t>(ret, m_Buffer.size() - m_Tail);
		memcpy(buf, m_Buffer.data() + m_Tail, part);
		if (part < ret)
			memcpy(buf + part, m_Buffer.data(), ret - part);
	}

	m_Tail = (m_Tail + ret) % m_Buffer.size();
	return ret;
}


void RingBuffer::Clear()
{
	m_Head = 1;
	m_Tail = 0;
}
