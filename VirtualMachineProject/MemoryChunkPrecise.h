#pragma once
#include "MemoryChunkBase.h"
#include "MemoryBitfieldManager.h"

template<class PoolAllocatorType>
class MemoryChunkPrecise :
	public MemoryChunkBase< PoolAllocatorType>
{
    MemoryBitfieldManager<64> bitfields;
    size_t elementSize;


public:
	MemoryChunkPrecise(PoolAllocatorType* memoryPool, size_t memorysize, size_t elementSize, size_t elementCount,
		std::unique_ptr<uint8_t[]>&& buf = std::unique_ptr<uint8_t[]>()) :
		MemoryChunkBase< PoolAllocatorType>(memoryPool, memorysize, std::move(buf)), elementSize(elementSize) {
		
		bitfields.setElementCount(elementCount);

		assert(memorysize == elementCount * elementSize);
	}

	bool full() const noexcept override { return bitfields.full(); }
	size_t elSize() const noexcept override {return elementSize;}
	size_t elSize(size_t index)const noexcept override { return 1; }
	uint8_t* alloc(size_t size) override
	{
		assert(size*4096 == this->memorysize);
		if (full())
			return nullptr;

		auto index = bitfields.AllocIndex();
		this->useCount++;

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

