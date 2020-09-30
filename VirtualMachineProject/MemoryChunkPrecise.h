#pragma once
#include "MemoryChunkBase.h"
#include "MemoryBitfieldManager.h"

template<class MemoryManagerType>
class MemoryChunkPrecise :
	public MemoryChunkBase< MemoryManagerType>
{
    MemoryBitfieldManager<64> bitfields;
    size_t elementSize;


public:
	MemoryChunkPrecise(MemoryManagerType& memorymanager, size_t memorysize, size_t elementSize, size_t elementCount) :
		MemoryChunkBase< MemoryManagerType>(memorymanager, memorysize), elementSize(elementSize) {
		
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

