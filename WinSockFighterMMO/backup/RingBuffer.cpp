#include "stdafx.h"
#include "RingBuffer.h"

namespace azely {

	RingBuffer::RingBuffer() : RingBuffer(RINGBUFFER_SIZE_DEFAULT)
	{

	}

	RingBuffer::RingBuffer(int bufferSize)
	{
		frontPosition_ = 0;
		rearPosition_ = 0;
		bufferSize_ = bufferSize;
		buffer_ = new char[bufferSize_ + 1];
	}

	RingBuffer::~RingBuffer()
	{
		delete buffer_;
		bufferSize_ = 0;
		frontPosition_ = 0;
		rearPosition_ = 0;
	}

	/**
	 * \brief Insert Buffer To Queue
	 * \param data inserting buffer
	 * \param requestSize inserting buffer size
	 * \param outEnqueueSize [out] successfully inserted size
	 * \param isPartialEnqueueAvailable if true, partial data insertion will be allowed
	 * \return is buffer insertion success
	 */
	bool RingBuffer::Enqueue(const char *data, int requestSize, int *outEnqueueSize, bool isPartialEnqueueAvailable)
	{
		if (requestSize < 0) return false;
		int freeSize = GetSizeFree();
		if (freeSize < requestSize)
		{
			if (!isPartialEnqueueAvailable) return false;
			requestSize = freeSize;
		}

		const int directEnqueueSize = GetSizeDirectEnqueueAble();
		if (directEnqueueSize < requestSize)
		{
			// 잘라서 인큐해야한다
			memcpy(buffer_ + rearPosition_, data, directEnqueueSize);
			MoveRearBufferRear(directEnqueueSize);
			memcpy(buffer_ + rearPosition_, data + directEnqueueSize, requestSize - directEnqueueSize);
			MoveRearBufferRear(requestSize - directEnqueueSize);
		} else
		{
			// 한번에 인큐가능
			memcpy(buffer_ + rearPosition_, data, requestSize);
			MoveRearBufferRear(requestSize);
		}

		if (outEnqueueSize != nullptr) *outEnqueueSize = requestSize;

		return true;
	}

	/**
	 * \brief Retrieve Buffer From Queue
	 * \param outData [out] data retrieve dest
	 * \param requestSize requesting buffer size
	 * \param outDequeueSize [out] successfully retrieved size
	 * \param isPartialDequeueAvailable if true, partial data retrieval will be allowed
	 * \return is buffer retrieval success
	 */
	bool RingBuffer::Dequeue(char *outData, int requestSize, int *outDequeueSize, bool isPartialDequeueAvailable)
	{
		if (requestSize < 0) return false;
		const int sizeUsed = GetSizeUsed();
		if (sizeUsed < requestSize)
		{
			if (!isPartialDequeueAvailable) return false;
			requestSize = sizeUsed;
		}

		const int directDequeueSize = GetSizeDirectDequeueAble();
		if (directDequeueSize < requestSize)
		{
			// 잘라서 디큐해야된다
			memcpy(outData, buffer_ + frontPosition_, directDequeueSize);
			MoveFrontBufferRear(directDequeueSize);
			memcpy(outData + directDequeueSize, buffer_ + frontPosition_, requestSize - directDequeueSize);
			MoveFrontBufferRear(requestSize - directDequeueSize);
		}
		else
		{
			// 한번에 디큐가능
			memcpy(outData, buffer_ + frontPosition_, requestSize);
			MoveFrontBufferRear(requestSize);
		}
		
		if (outDequeueSize != nullptr) *outDequeueSize = requestSize;

		return true;
	}


	bool RingBuffer::Peek(char *outData, int requestSize, int *outPeekSize, bool isPartialPeekAvailable)
	{
		if (requestSize < 0) return false;
		const int sizeUsed = GetSizeUsed();
		if (sizeUsed < requestSize)
		{
			if (!isPartialPeekAvailable) return false;
			requestSize = sizeUsed;
		}

		const int directDequeueSize = GetSizeDirectDequeueAble();
		if (directDequeueSize < requestSize)
		{
			// 잘라서 읽어야된다
			memcpy(outData, buffer_ + frontPosition_, directDequeueSize);
			memcpy(outData + directDequeueSize, buffer_, requestSize - directDequeueSize);
		}
		else
		{
			// 한번에 읽기가능
			memcpy(outData, buffer_ + frontPosition_, requestSize);
		}

		if (outPeekSize != nullptr) *outPeekSize = requestSize;

		return true;
	}

}