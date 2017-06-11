#pragma once

#include <vector>
#include <atomic>
#include <cstddef>

using std::size_t;

// This is a lock-free content-aware ring buffer.
// It's thread-safe if there is one writer and one reader.
// TODO: Better guarantees for multiple readers and writers.
// TODO: Use less conservative memory ordering guarantees.
template <typename T>
class RingBuffer
{
public:
	explicit RingBuffer(size_t capacity = 0) : data(capacity + 1), len(capacity + 1)
	{
	}
	
	size_t capacity() const
	{
		return len - 1;
	}
	
	size_t free() const
	{
		return len - size();
	}
	
	size_t size() const
	{
		size_t r = read.load();
		size_t w = write.load();
		return (w - r + len) % len;
	}
	
	bool full() const
	{
		size_t r = read.load();
		size_t w = write.load();
		return (w + 1) % len == r;
	}
	
	bool empty() const
	{
		size_t r = read.load();
		size_t w = write.load();
		return w == r;
	}
	
	bool pop(T& x)
	{
		size_t r = read.load();
		size_t w = write.load();
		if (w == r)
			return false;
		
		x = data[r];
		
		// Atomically increment and modulo `read`.
		size_t new_r;
		do
		{
			new_r = (r + 1) % len;
		} while (!read.compare_exchange_weak(r, new_r));
		
		return true;
	}

	bool push(const T& x)
	{
		size_t w = write.load();
		size_t r = read.load();
		if ((w + 1) % len == r)
			return false;
		
		data[r] = x;
		
		// Atomically increment and modulo `write`.
		size_t new_w;
		do
		{
			new_w = (w + 1) % len;
		} while (!write.compare_exchange_weak(w, new_w));

		return true;
	}
	
private:
	std::vector<T> data;
	
	// Actual length of data. It is one more than the capacity because
	// it is never 100% full (so we can easily distinguish the full and
	// empty cases).
	size_t len;
	
	// Where to write to next. If write = read then the buffer is empty.
	// If write = read - 1 then it is full.
	std::atomic<size_t> write{0};
	// Where to read from next.
	std::atomic<size_t> read{0};
};
