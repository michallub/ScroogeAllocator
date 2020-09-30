#pragma once
#include "MemoryChunkBase.h"
#include <cassert>

template<class MemoryManagerType>
class MemoryChunkAnySize :
    public MemoryChunkBase<MemoryManagerType>
{
	size_t elementSize;
public:
	MemoryChunkAnySize(MemoryManagerType& memoryPool, size_t memorysize) :
		MemoryChunkBase< MemoryManagerType>(memoryPool, memorysize), elementSize(memorysize)
	{

	}

	bool full() const noexcept override { return this->useCount; }
	uint8_t* alloc(size_t size) override {
		assert(size == this->memorysize);
		if (this->empty()) {
			++this->useCount;
			elementSize = size;
			return this->chunk;// createPointer();
		}
		return nullptr;
	};
	DealocResult dealloc(uint8_t* ptr) override
	{
		if ((this->chunk <= ptr) && (this->chunk + elementSize > ptr))
		{
			if (this->useCount != 1)
			{
				assert(false);
				return DealocResult::DEALOC_NOT_ALLOCATED;
			}
			elementSize = this->memorysize;
			--this->useCount;
			return DealocResult::WAS_FULL_IS_EMPTY;
		}
		return DealocResult::BAD_ADDRESS;
	}
	~MemoryChunkAnySize() = default;
	size_t elSize() const noexcept override {
		return this->memorysize;
	}
	size_t elSize(size_t index) const noexcept override {
		//assert(false);
		return elementSize;
	}
};

