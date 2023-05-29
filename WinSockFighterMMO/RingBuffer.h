#pragma once

#include <cstdlib>
#include <memory>

#define RINGBUFFER_SIZE_DEFAULT 20480

namespace azely {

	class RingBuffer
	{

	public:
		RingBuffer();
		RingBuffer(int bufferSize);
		~RingBuffer();

		int GetSizeTotal() const;
		int GetSizeFree() const;
		int GetSizeUsed() const;
		int GetSizeDirectEnqueueAble() const;
		int GetSizeDirectDequeueAble() const;

		bool Enqueue(const char *data, int requestSize, int *outEnqueueSize, bool isPartialEnqueueAvailable = false);
		bool Dequeue(char *outData, int requestSize, int *outDequeueSize, bool isPartialDequeueAvailable = true, bool isPeekMode = false);
		bool Peek(char *outData, int requestSize, int *outPeekSize, bool isPartialPeekAvailable = true);

		bool IsEmpty() const;
		bool IsFull() const;
		void ClearBuffer();
		char *GetFrontBuffer() const;
		char *GetRearBuffer() const;
		bool MoveFrontBufferRear(int moveSize);
		bool MoveRearBufferRear(int moveSize);

	private:
		char *frontPosition_ = nullptr;
		char *rearPosition_ = nullptr;
		char *buffer_ = nullptr;
		char *bufferEndPosition_ = nullptr;
		int bufferSize_ = 0;
		int usedSize_ = 0;

		char *GetStartBuffer() const;
		char *GetEndBuffer() const;
	};

}