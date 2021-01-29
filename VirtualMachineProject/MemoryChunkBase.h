#pragma once
#include <memory>
#include <iostream>
#include <list>
#include "DealocResult.h"
#include "MemoryCustomAllocator.h"



struct MemoryMemInfo
{
	uint8_t* buferAddress = nullptr;
	uint8_t* chunkAddress = nullptr;
	size_t chunkSize = 0;
	size_t allocatedSize = 0;
	size_t requestedSize = 0;
	bool isAllocated = false;
};




class MemoryChunkBase
{
	friend class Memory;
protected:
	uint8_t* chunk = nullptr;
	//PoolAllocatorType* memoryPool = 0;
	MemoryCustomAllocator& memoryallocs;
	size_t memorysize;
	size_t useCount = 0;
public:
	
	std::list<MemoryChunkBase*, MemoryAllocatorLightTyped<MemoryChunkBase* >> freelistNode;

	MemoryChunkBase(MemoryCustomAllocator& memoryallocs, size_t memorysize) :
		memoryallocs(memoryallocs),
		chunk((uint8_t*)memoryallocs.allocPoolMemory(memorysize)),
		memorysize(memorysize),
		freelistNode(memoryallocs.getInternalAllocatorLightTyped<MemoryChunkBase*>())
	{
		freelistNode.push_front(this);
	}
	size_t getUseCount() const noexcept { return useCount; }
	bool empty() const noexcept { return !useCount; }
	virtual bool full() const noexcept = 0;
	virtual uint8_t* alloc(size_t size) = 0;
	virtual DealocResult dealloc(uint8_t* ptr) = 0;
	virtual MemoryMemInfo getInfo(uint8_t* ptr) const noexcept = 0;
	virtual ~MemoryChunkBase() {
		memoryallocs.deallocPoolMemory(chunk, memorysize);
		chunk = nullptr;
	};
	virtual size_t elSize() const noexcept = 0;
	virtual size_t elSize(size_t index) const noexcept = 0;
	size_t getMemorySize() const noexcept { return memorysize; }
	size_t getAddresAsInt() const noexcept { return reinterpret_cast<size_t>(chunk); }
	MemoryCustomAllocator& getAllocators() noexcept  {
		return memoryallocs;
	}
	
};



