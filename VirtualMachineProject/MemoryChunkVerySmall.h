#pragma once
#include "MemoryChunkBase.h"
#include "MemoryBitfieldManager.h"
#include "WastedBytesCounter.h"

template <class PoolAllocatorType, size_t BITFIELDSSIZE, size_t ELEMENTSCOUNT, size_t WASTEDBYTESBITSCOUNT>
class MemoryChunkTemplate :
    public MemoryChunkBase<PoolAllocatorType>
{
	MemoryBitfieldManager<BITFIELDSSIZE> bitfields;
	WastedBytesCounter<ELEMENTSCOUNT, WASTEDBYTESBITSCOUNT> wastedbytes;
	size_t elementSize;

	

public:
	MemoryChunkTemplate(PoolAllocatorType* memoryPool, size_t memorysize, size_t elementSize, size_t elementCount,
		std::unique_ptr<uint8_t[]>&& buf = std::unique_ptr<uint8_t[]>()) :
				MemoryChunkBase<PoolAllocatorType>(memoryPool, memorysize, std::move(buf)), elementSize(elementSize)
	{
		bitfields.setElementCount(elementCount);

		assert(memorysize == elementCount * elementSize);
	}

	bool full() const noexcept override { return bitfields.full(); }
	size_t elSize() const noexcept override { return elementSize; }
	size_t elSize(size_t index) const noexcept override { return elementSize- wastedbytes.getWastedBytes(index); }

	uint8_t* alloc(size_t size) override
	{
		if (full())
			return nullptr;

		auto index = bitfields.AllocIndex();
		this->useCount++;

		wastedbytes.setWastedBytes(index, elementSize - size);

		return this->chunk.get() + elementSize * index;
	}
	virtual DealocResult dealloc(uint8_t* ptr)
	{
		auto wasfull = full();
		auto addr = (uint8_t*)ptr - this->chunk.get();
		if (addr < 0 || addr >= this->memorysize)
			return DealocResult::BAD_ADDRESS;
		size_t index = addr / elementSize;
		auto result = bitfields.DeallocIndex(index);
		this->useCount--;
		return result;
	}
};

template <class PoolAllocatorType>
using MemoryChunkVerySmall = MemoryChunkTemplate<PoolAllocatorType, 32, 2048, 2>;

template <class PoolAllocatorType>
using MemoryChunkSmall = MemoryChunkTemplate<PoolAllocatorType, 32, 512, 4>;

template <class PoolAllocatorType>
using MemoryChunkMedium = MemoryChunkTemplate<PoolAllocatorType, 1, 128, 8>;

template <class PoolAllocatorType>
using MemoryChunkLarge = MemoryChunkTemplate<PoolAllocatorType, 1, 8, 16>;
