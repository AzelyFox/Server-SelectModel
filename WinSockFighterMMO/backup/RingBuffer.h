#pragma once

#include <cstdlib>
#include <memory>

#define RINGBUFFER_SIZE_DEFAULT 10240

namespace azely {

	class RingBuffer
	{

	public:
		RingBuffer();
		RingBuffer(int bufferSize);
		~RingBuffer();

		bool Enqueue(const char *data, int requestSize, int *outEnqueueSize, bool isPartialEnqueueAvailable = false);
		bool Dequeue(char *outData, int requestSize, int *outDequeueSize, bool isPartialDequeueAvailable = true);
		bool Peek(char *outData, int requestSize, int *outPeekSize, bool isPartialPeekAvailable = true);

		/**
		 * \brief move front position to rear (SAME EFFECT AS DEQUEUE)
		 * \param moveSize forward requesting size
		 * \return is rear pointer move success
		 */
		__inline bool MoveFrontBufferRear(int moveSize)
		{
			frontPosition_ += moveSize;
			frontPosition_ %= bufferSize_ + 1;
			return true;
		}

		/**
		 * \brief move rear position to rear (SAME EFFECT AS ENQUEUE)
		 * \param moveSize backward requesting size
		 * \return is rear pointer move success
		 */
		__inline bool MoveRearBufferRear(int moveSize)
		{
			rearPosition_ += moveSize;
			rearPosition_ %= bufferSize_ + 1;
			return true;
		}

		/**
		 * \brief Clear Buffer By Adjusting Position
		 */
		__inline void ClearBuffer()
		{
			rearPosition_ = 0;
			frontPosition_ = 0;
		}

		/**
		 * \brief Is Queue Empty
		 * \return Queue Is Empty
		 */
		__inline bool IsEmpty() const
		{
			return frontPosition_ == rearPosition_;
		}

		__inline bool IsFull() const
		{
			if (rearPosition_ == bufferSize_ - 1 && frontPosition_ == 0)
			{
				return true;
			}
			if (rearPosition_ + 1 == frontPosition_) return true;
			return false;
		}

		__inline int GetSizeTotal() const
		{
			return bufferSize_;
		}

		__inline int GetSizeUsed() const
		{
			return (rearPosition_ - frontPosition_ + bufferSize_ + 1) % (bufferSize_ + 1);
		}

		__inline int GetSizeFree() const
		{
			return bufferSize_ - GetSizeUsed();
		}

		/**
		 * \brief Get Buffer Used Size
		 * \return buffer using size
		 */
		/*
		__inline int GetSizeUsed() const
		{
			return usedSize_;
		}
		*/

		__inline int GetSizeDirectEnqueueAble() const
		{
			if (rearPosition_ >= frontPosition_)
			{
				// 정상적인 이쁜 상황
				if (frontPosition_ == 0)
				{
					// 링버퍼의 첫 버퍼가 차있으니, 마지막 버퍼가 비어야 한다
					// 쓰기 포지션부터 버퍼 마지막 위치 - 1 까지 다이렉트 인큐 가능 
					return bufferSize_ - rearPosition_;
				} else
				{
					// 링버퍼의 첫 버퍼가 비어있으니, 마지막 버퍼까지 채워도 된다.
					// 쓰기 포지션부터 버퍼 마지막 위치까지 다이렉트 인큐 가능
					return bufferSize_ - rearPosition_ + 1;
				}
			} else
			{
				// 링버퍼 앞뒤로 데이터가 존재하는 상황
				// 쓰기 포지션부터 읽기 포지션 - 1 까지 다이렉트 인큐 가능
				return frontPosition_ - rearPosition_ - 1;
			}
		}

		__inline int GetSizeDirectDequeueAble() const
		{
			if (rearPosition_ > frontPosition_)
			{
				// 정상적인 이쁜 상황
				// 읽기 포지션부터 쓰기 포지션 - 1 위치까지 다이렉트로 뺄 수 있음
				return rearPosition_ - frontPosition_;
			} else if (rearPosition_ == frontPosition_)
			{
				// 읽기 포지션과 쓰기 포지션이 같다면, 빈 큐
				return 0;
			} else
			{
				// 링 버퍼의 앞뒤로 데이터가 존재하는 상황
				// 읽기 포지션부터 링 버퍼의 버퍼 마지막 위치까지 다이렉트로 뺄 수 있음
				return bufferSize_ - frontPosition_ + 1;
			}
		}

		/**
		 * \brief get Queue First Item Position (READ POSITION)
		 * \return queue's first item pointer
		 */
		__inline char *GetFrontBuffer() const
		{
			return buffer_ + frontPosition_;
		}

		/**
		 * \brief get Queue Writing Position (Last Item Position+1) (WRITE POSITION)
		 * \return queue's writing pointer
		 */
		__inline char *GetRearBuffer() const
		{
			return buffer_ + rearPosition_;
		}

	private:
		int frontPosition_ = 0;
		int rearPosition_ = 0;
		char *buffer_ = nullptr;
		int bufferSize_ = 0;
	};

}