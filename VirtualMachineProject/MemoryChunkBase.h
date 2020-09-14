#pragma once
#include <memory>
#include "DealocResult.h"
#include "MemoryManagerBase.h"
#include <iostream>

template<class PoolAllocatorType>
class MemoryChunkBase
{
	friend class Memory;
protected:
	std::unique_ptr<uint8_t[]> chunk;
	PoolAllocatorType* memoryPool = 0;
	size_t memorysize;
	size_t useCount = 0;

public:
	

	MemoryChunkBase(PoolAllocatorType* memoryPool, size_t memorysize,
		std::unique_ptr<uint8_t[]>&& buf = std::unique_ptr<uint8_t[]>()) :
		memoryPool(memoryPool),
		chunk(buf ? std::move(buf) : 
			(memoryPool ? std::unique_ptr<uint8_t[]>(memoryPool->alloc(memorysize)):
				std::make_unique< uint8_t[]>(memorysize))),
				memorysize(memorysize) 
	{
	}
	bool empty() { return !useCount; }
	virtual bool full() const noexcept = 0;
	virtual uint8_t* alloc(size_t size) = 0;
	virtual DealocResult dealloc(uint8_t* ptr) = 0;
	virtual ~MemoryChunkBase() {
		if (memoryPool)
			memoryPool->dealloc(chunk.release());
	};
	virtual size_t elSize() const noexcept = 0;
	virtual size_t elSize(size_t index) const noexcept = 0;
	size_t getMemorySize() { return memorysize; }
	size_t getAddresAsInt() const noexcept { return reinterpret_cast<size_t>(chunk.get()); }
};



