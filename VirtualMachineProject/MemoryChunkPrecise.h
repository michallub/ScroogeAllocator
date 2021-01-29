#pragma once
#include "MemoryChunkBase.h"
#include "MemoryBitfieldManager.h"

class MemoryChunkPrecise :
	public MemoryChunkBase
{
    MemoryBitfieldManager<64> bitfields;
    size_t elementSize;


public:
	MemoryChunkPrecise(MemoryCustomAllocator& memoryallocs, size_t memorysize, size_t elementSize, size_t elementCount) :
		MemoryChunkBase(memoryallocs, memorysize), elementSize(elementSize) {
		
		bitfields.setElementCount(elementCount);

		assert(memorysize >= elementCount * elementSize);
	}

	bool full() const noexcept override { return bitfields.full(); }
	size_t elSize() const noexcept override {return elementSize;}
	size_t elSize(size_t index)const noexcept override { return 1; }
	uint8_t* alloc(size_t size) override
	{
		//assert(size* bitfields.bitusedmask == this->memorysize);
		if (full())
			return nullptr;

		auto index = bitfields.AllocIndex();
		this->useCount++;

		return this->chunk + elementSize * index;
	}
	DealocResult dealloc(uint8_t* ptr) override
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
		info.buferAddress = chunk + index* elementSize;
		info.chunkAddress = chunk;
		info.chunkSize = memorysize;
		info.allocatedSize = elementSize;
		if (bitfields.isAllocated(index))
		{
			info.isAllocated = true;
			info.requestedSize = elementSize;
		}
		return info;
	}
};

