#pragma once
#include "MemoryChunkBase.h"
#include "MemoryBitfieldManager.h"
#include "WastedBytesCounter.h"

template <class MemoryManagerType, size_t BITFIELDSSIZE, size_t ELEMENTSCOUNT, size_t WASTEDBYTESBITSCOUNT>
class MemoryChunkTemplate :
    public MemoryChunkBase<MemoryManagerType>
{
	MemoryBitfieldManager<BITFIELDSSIZE> bitfields;
	WastedBytesCounter<ELEMENTSCOUNT, WASTEDBYTESBITSCOUNT> wastedbytes;
	size_t elementSize;

	

public:
	MemoryChunkTemplate(MemoryManagerType& memorymanager, size_t memorysize, size_t elementSize, size_t elementCount) :
				MemoryChunkBase<MemoryManagerType>(memorymanager, memorysize), elementSize(elementSize)
	{
		bitfields.setElementCount(elementCount);

		assert(memorysize >= elementCount * elementSize);
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

		return this->chunk + elementSize * index;
	}
	virtual DealocResult dealloc(uint8_t* ptr)
	{
		auto wasfull = full();
		auto addr = (uint8_t*)ptr - this->chunk;
		if (addr < 0 || addr >= this->memorysize)
			return DealocResult::BAD_ADDRESS;
		size_t index = addr / elementSize;
		auto result = bitfields.DeallocIndex(index);
		this->useCount--;
		return result;
	}
};

template <class MemoryManagerType>
using MemoryChunkVerySmall = MemoryChunkTemplate<MemoryManagerType, 32, 2048, 2>;

template <class MemoryManagerType>
using MemoryChunkSmall = MemoryChunkTemplate<MemoryManagerType, 16, 1024, 4>;

template <class MemoryManagerType>
using MemoryChunkMedium = MemoryChunkTemplate<MemoryManagerType, 1, 64, 8>;

template <class MemoryManagerType>
using MemoryChunkLarge = MemoryChunkTemplate<MemoryManagerType, 1, 16, 16>;

//const int asdMemoryChunkLarge = sizeof(MemoryChunkLarge<MemoryPoolAllocatorInterface>);
//const int asdMemoryChunkMedium = sizeof(MemoryChunkMedium<MemoryPoolAllocatorInterface>);
//const int asdMemoryChunkSmall = sizeof(MemoryChunkSmall<MemoryPoolAllocatorInterface>);
//const int asdMemoryChunkVerySmall = sizeof(MemoryChunkVerySmall<MemoryPoolAllocatorInterface>);
