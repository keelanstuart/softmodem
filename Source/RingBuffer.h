#include <atomic>
#include <vector>
#include <functional>

#pragma once

class RingBuffer
{

protected:
	std::vector<uint8_t> m_Buffer;
	std::atomic<size_t> m_Head, m_Tail;

	using Callback = std::function<void()>;
	Callback m_Available;
	Callback m_Full;

public:
	RingBuffer(size_t sz, Callback available, Callback full);

	size_t Write(const uint8_t *buf, size_t buflen);

	size_t Read(uint8_t *buf, size_t buflen);

	void Clear();

};
