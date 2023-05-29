#include "stdafx.h"
#include "RingBuffer.h"

namespace azely {

	RingBuffer::RingBuffer() : RingBuffer(RINGBUFFER_SIZE_DEFAULT)
	{

	}

	RingBuffer::RingBuffer(int bufferSize)
	{
		usedSize_ = 0;
		bufferSize_ = bufferSize;
		buffer_ = new char[bufferSize_ + 1];
		bufferEndPosition_ = buffer_ + bufferSize_;
		frontPosition_ = buffer_;
		rearPosition_ = buffer_;
	}

	RingBuffer::~RingBuffer()
	{
		delete buffer_;
		bufferSize_ = 0;
	}

	int RingBuffer::GetSizeTotal() const
	{
		return bufferSize_;
	}

	int RingBuffer::GetSizeFree() const
	{
		return GetSizeTotal() - GetSizeUsed();
	}

	/**
	 * \brief Get Buffer Used Size
	 * \return buffer using size
	 */
	int RingBuffer::GetSizeUsed() const
	{
		return usedSize_;
	}

	int RingBuffer::GetSizeDirectEnqueueAble() const
	{
		// 만일 FrontBuffer == StartBuffer 라면, 버퍼의 맨 뒤칸은 사용 불가
		if (GetFrontBuffer() == GetStartBuffer())
		{
			return (GetEndBuffer() - GetRearBuffer());
		}
		if (GetFrontBuffer() > GetRearBuffer())
		{
			// 읽기 인덱스 전칸까지 사용 가능하므로, 맨 뒤칸 인덱스 - 쓰기 인덱스 - 1
			return (GetFrontBuffer() - GetRearBuffer() - 1);
		}
		else
		{
			// 버퍼의 맨 뒤칸까지 사용 가능하므로, 읽기 인덱스 - 쓰기 인덱스 + 1
			return (GetEndBuffer() - GetRearBuffer() + 1);
		}
	}

	int RingBuffer::GetSizeDirectDequeueAble() const
	{
		/*
		// 쓰기 인덱스 - 버퍼 시작 인덱스
		return (GetRearBuffer() - GetStartBuffer());
		*/
		if (GetFrontBuffer() > GetRearBuffer())
		{
			return (GetEndBuffer() - GetFrontBuffer()) + 1;
		}
		return (GetRearBuffer() - GetFrontBuffer());
	}

	/**
	 * \brief get Queue First Item Position (READ POSITION)
	 * \return queue's first item pointer
	 */
	char *RingBuffer::GetFrontBuffer() const
	{
		return frontPosition_;
	}

	/**
	 * \brief get Queue Writing Position (Last Item Position+1) (WRITE POSITION)
	 * \return queue's writing pointer
	 */
	char *RingBuffer::GetRearBuffer() const
	{
		return rearPosition_;
	}

	/**
	 * \brief get Buffer Start Position
	 * \return queue's buffer start pointer
	 */
	char *RingBuffer::GetStartBuffer() const
	{
		return buffer_;
	}

	/**
	 * \brief get Buffer End Position
	 * \return queue's buffer end pointer
	 */
	char *RingBuffer::GetEndBuffer() const
	{
		return bufferEndPosition_;
		//return buffer_ + bufferSize_;
	}

	/**
	 * \brief move front position to rear (SAME EFFECT AS DEQUEUE)
	 * \param moveSize forward requesting size
	 * \return is rear pointer move success
	 */
	bool RingBuffer::MoveFrontBufferRear(int moveSize)
	{
		int removedSize = 0;

		// HANDLE PARAMETER EXCEPTIONS
		if (IsEmpty() || moveSize <= 0)
		{
			return false;
		}

		// CHECK DATA WILL BE SPLIT
		if (moveSize > GetSizeDirectDequeueAble())
		{
			// DATA RETRIEVAL WILL BE SPLIT
			int retrievalSize = 0;
			char *frontTempBuffer = GetFrontBuffer();

			// GET DATA FRONT ~ END
			retrievalSize += (GetEndBuffer() - frontTempBuffer) + 1;
			removedSize += retrievalSize;
			frontTempBuffer += removedSize;
			if (frontTempBuffer > GetEndBuffer()) {
				frontTempBuffer -= (bufferSize_ + 1);
			}
			frontPosition_ += retrievalSize;
			if (frontPosition_ > GetEndBuffer()) {
				frontPosition_ -= (bufferSize_ + 1);
			}
			// GET DATA START ~ REQUEST SIZE
			retrievalSize = moveSize - removedSize;
			removedSize += retrievalSize;
			frontPosition_ += retrievalSize;
		}
		else
		{
			// DATA RETRIEVAL IS NOT SPLIT
			removedSize += moveSize;
			frontPosition_ += moveSize;
			if (frontPosition_ > GetEndBuffer()) {
				frontPosition_ -= (bufferSize_ + 1);
			}
		}

		usedSize_ -= removedSize;
		return true;
	}

	/**
	 * \brief move rear position to rear (SAME EFFECT AS ENQUEUE)
	 * \param moveSize backward requesting size
	 * \return is rear pointer move success
	 */
	bool RingBuffer::MoveRearBufferRear(int moveSize)
	{
		int addedSize = 0;

		// HANDLE PARAMETER EXCEPTIONS
		if (IsFull() || moveSize <= 0)
		{
			return false;
		}

		// CHECK DATA WILL BE SPLIT
		if (moveSize > GetSizeDirectEnqueueAble())
		{
			// DATA WILL BE SPLIT
			const int splitLeftSize = GetSizeDirectEnqueueAble();
			const int splitRightSize = moveSize - splitLeftSize;

			// INSERT PARTIAL LEFT
			rearPosition_ += splitLeftSize;
			if (rearPosition_ > GetEndBuffer()) {
				rearPosition_ -= (bufferSize_ + 1);
			}
			addedSize += splitLeftSize;

			// INSERT PARTIAL RIGHT
			rearPosition_ += splitRightSize;
			addedSize += splitRightSize;
		}
		else
		{
			// DATA WILL NOT SPLIT
			rearPosition_ += moveSize;
			if (rearPosition_ > GetEndBuffer()) {
				rearPosition_ -= (bufferSize_ + 1);
			}
			addedSize += moveSize;
		}

		usedSize_ += addedSize;
		return true;
	}

	/**
	 * \brief Clear Buffer By Adjusting Position
	 */
	void RingBuffer::ClearBuffer()
	{
		rearPosition_ = GetStartBuffer();
		frontPosition_ = GetStartBuffer();
		usedSize_ = 0;
	}

	/**
	 * \brief Is Queue Empty
	 * \return Queue Is Empty
	 */
	bool RingBuffer::IsEmpty() const
	{
		return GetFrontBuffer() == GetRearBuffer();
	}

	/**
	 * \brief Is Queue Full
	 * \return Queue Is Full
	 */
	bool RingBuffer::IsFull() const
	{
		if (GetRearBuffer() == GetEndBuffer() && GetFrontBuffer() == GetStartBuffer())
		{
			return true;
		}
		if (GetRearBuffer() + 1 == GetFrontBuffer()) return true;
		return false;
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
		int addedSize = 0;
		int handleSize = min(requestSize, GetSizeFree());

		// HANDLE PARAMETER EXCEPTIONS
		if (IsFull() || requestSize <= 0 || data == nullptr)
		{
			if (outEnqueueSize != nullptr) memcpy(outEnqueueSize, &addedSize, sizeof(int));
			return false;
		}

		// IS PARTIAL RESTRICTED MODE, CHECK INSERTION IS AVAILABLE
		if (!isPartialEnqueueAvailable && handleSize != requestSize)
		{
			if (outEnqueueSize != nullptr) memcpy(outEnqueueSize, &addedSize, sizeof(int));
			return false;
		}

		// CHECK DATA WILL BE SPLIT
		if (handleSize > GetSizeDirectEnqueueAble())
		{
			// DATA WILL BE SPLIT
			const int splitLeftSize = GetSizeDirectEnqueueAble();
			const int splitRightSize = handleSize - splitLeftSize;
			const char *dataLeft = data;
			const char *dataRight = data + splitLeftSize;

			// INSERT PARTIAL LEFT
			memcpy(GetRearBuffer(), dataLeft, splitLeftSize);
			rearPosition_ += splitLeftSize;
			if (rearPosition_ > GetEndBuffer()) {
				rearPosition_ -= (bufferSize_ + 1);
			}
			addedSize += splitLeftSize;

			// INSERT PARTIAL RIGHT
			memcpy(GetRearBuffer(), dataRight, splitRightSize);
			rearPosition_ += splitRightSize;
			addedSize += splitRightSize;
		}
		else
		{
			// DATA WILL NOT SPLIT
			memcpy(GetRearBuffer(), data, handleSize);
			rearPosition_ += handleSize;
			if (rearPosition_ > GetEndBuffer()) {
				rearPosition_ -= (bufferSize_ + 1);
			}
			addedSize += handleSize;
		}

		usedSize_ += addedSize;
		if (outEnqueueSize != nullptr) memcpy(outEnqueueSize, &addedSize, sizeof(int));
		return true;
	}

	/**
	 * \brief Retrieve Buffer From Queue
	 * \param outData [out] data retrieve dest
	 * \param requestSize requesting buffer size
	 * \param outDequeueSize [out] successfully retrieved size
	 * \param isPartialDequeueAvailable if true, partial data retrieval will be allowed
	 * \param isPeekMode if true, just copy to dest and do not remove from queue
	 * \return is buffer retrieval success
	 */
	bool RingBuffer::Dequeue(char *outData, int requestSize, int *outDequeueSize, bool isPartialDequeueAvailable, bool isPeekMode)
	{
		int removedSize = 0;
		int handleSize = min(requestSize, GetSizeUsed());

		// HANDLE PARAMETER EXCEPTIONS
		if (IsEmpty() || requestSize <= 0 || outData == nullptr)
		{
			if (outDequeueSize != nullptr) memcpy(outDequeueSize, &removedSize, sizeof(int));
			return false;
		}

		// IS PARTIAL RESTRICTED MODE, CHECK REMOVAL IS AVAILABLE
		if (!isPartialDequeueAvailable && handleSize != requestSize)
		{
			if (outDequeueSize != nullptr) memcpy(outDequeueSize, &removedSize, sizeof(int));
			return false;
		}

		// CHECK DATA WILL BE SPLIT
		if (handleSize > GetSizeDirectDequeueAble())
		{
			// DATA RETRIEVAL WILL BE SPLIT
			int retrievalSize = 0;
			char *frontTempBuffer = GetFrontBuffer();

			// GET DATA FRONT ~ END
			retrievalSize += (GetEndBuffer() - frontTempBuffer) + 1;
			memcpy(outData, frontTempBuffer, retrievalSize);
			removedSize += retrievalSize;
			frontTempBuffer += removedSize;
			if (frontTempBuffer > GetEndBuffer()) {
				frontTempBuffer -= (bufferSize_ + 1);
			}
			if (!isPeekMode) {
				frontPosition_ += retrievalSize;
				if (frontPosition_ > GetEndBuffer()) {
					frontPosition_ -= (bufferSize_ + 1);
				}
			}
			// GET DATA START ~ REQUEST SIZE
			retrievalSize = handleSize - removedSize;
			memcpy(outData + removedSize, frontTempBuffer, retrievalSize);
			removedSize += retrievalSize;
			if (!isPeekMode)
			{
				frontPosition_ += retrievalSize;
			}
		}
		else
		{
			// DATA RETRIEVAL IS NOT SPLIT
			memcpy(outData, GetFrontBuffer(), handleSize);
			removedSize += handleSize;
			if (!isPeekMode)
			{
				frontPosition_ += handleSize;
				if (frontPosition_ > GetEndBuffer()) {
					frontPosition_ -= (bufferSize_ + 1);
				}
			}
		}

		if (!isPeekMode) usedSize_ -= removedSize;
		if (outDequeueSize != nullptr) memcpy(outDequeueSize, &removedSize, sizeof(int));
		return true;

	}

	bool RingBuffer::Peek(char *outData, int requestSize, int *outPeekSize, bool isPartialPeekAvailable)
	{
		return Dequeue(outData, requestSize, outPeekSize, isPartialPeekAvailable, true);
	}

}