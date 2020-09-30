#pragma once
#include <memory>
#include "DealocResult.h"
#include "MemoryManagerBase.h"
#include <iostream>

template<class MemoryManagerType>
class MemoryChunkBase
{
	friend class Memory;
protected:
	uint8_t* chunk = nullptr;
	//PoolAllocatorType* memoryPool = 0;
	MemoryManagerType& memorymanager;
	size_t memorysize;
	size_t useCount = 0;

public:
	

	MemoryChunkBase(MemoryManagerType& memorymanager, size_t memorysize) :
		memorymanager(memorymanager),
		chunk((uint8_t*)memorymanager.allocPoolMemory(memorysize)),
				memorysize(memorysize) 
	{
	}
	size_t getUseCount() { return useCount; }
	bool empty() { return !useCount; }
	virtual bool full() const noexcept = 0;
	virtual uint8_t* alloc(size_t size) = 0;
	virtual DealocResult dealloc(uint8_t* ptr) = 0;
	virtual ~MemoryChunkBase() {
		memorymanager.deallocPoolMemory(chunk);
		chunk = nullptr;
	};
	virtual size_t elSize() const noexcept = 0;
	virtual size_t elSize(size_t index) const noexcept = 0;
	size_t getMemorySize() { return memorysize; }
	size_t getAddresAsInt() const noexcept { return reinterpret_cast<size_t>(chunk); }
	MemoryManagerType& getMemoryManager() noexcept  {
		return memorymanager;
	}
};



