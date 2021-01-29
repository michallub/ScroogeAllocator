#pragma once
#include "MemoryChunkBase.h"
#include "MemoryBitfieldManager.h"
#include "WastedBytesCounter.h"

template <size_t BITFIELDSSIZE, size_t ELEMENTSCOUNT, size_t WASTEDBYTESBITSCOUNT>
class MemoryChunkTemplate :
    public MemoryChunkBase
{
	MemoryBitfieldManager<BITFIELDSSIZE> bitfields;
	WastedBytesCounter<ELEMENTSCOUNT, WASTEDBYTESBITSCOUNT> wastedbytes;
	size_t elementSize;

	

public:
	MemoryChunkTemplate(MemoryCustomAllocator& memoryallocs, size_t memorysize, size_t elementSize, size_t elementCount) :
				MemoryChunkBase(memoryallocs, memorysize), elementSize(elementSize)
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
	MemoryMemInfo getInfo(uint8_t* ptr) const noexcept override {
		auto addr = (uint8_t*)ptr - this->chunk;
		if (addr < 0 || addr >= this->memorysize)
			return MemoryMemInfo();
		size_t index = addr / elementSize;
		MemoryMemInfo info;
		info.buferAddress = chunk + index * elementSize;
		info.chunkAddress = chunk;
		info.chunkSize = memorysize;
		info.allocatedSize = elementSize;
		if (bitfields.isAllocated(index))
		{
			info.isAllocated = true;
			info.requestedSize = elementSize - wastedbytes.getWastedBytes(index);
		}
		return info;
	}
};


using MemoryChunkVerySmall = MemoryChunkTemplate<32, 2048, 2>;
using MemoryChunkSmall = MemoryChunkTemplate<16, 1024, 4>;
using MemoryChunkMedium = MemoryChunkTemplate<1, 64, 8>;
using MemoryChunkLarge = MemoryChunkTemplate<1, 16, 16>;

//const int asdMemoryChunkLarge = sizeof(MemoryChunkLarge<MemoryPoolAllocatorInterface>);
//const int asdMemoryChunkMedium = sizeof(MemoryChunkMedium<MemoryPoolAllocatorInterface>);
//const int asdMemoryChunkSmall = sizeof(MemoryChunkSmall<MemoryPoolAllocatorInterface>);
//const int asdMemoryChunkVerySmall = sizeof(MemoryChunkVerySmall<MemoryPoolAllocatorInterface>);
