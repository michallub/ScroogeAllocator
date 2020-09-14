#pragma once
#include "MemoryChunkBase.h"
#include <cassert>

template<class PoolAllocatorType>
class MemoryChunkAnySize :
    public MemoryChunkBase<PoolAllocatorType>
{
public:
	MemoryChunkAnySize(PoolAllocatorType* memoryPool, size_t memorysize, std::unique_ptr<uint8_t[]>&& buf = std::unique_ptr<uint8_t[]>()) :
		MemoryChunkBase< PoolAllocatorType>(memoryPool, memorysize, std::move(buf))
	{

	}

	bool full() const noexcept override { return this->useCount; }
	uint8_t* alloc(size_t size) override {
		assert(size == this->memorysize);
		if (this->empty()) {
			++this->useCount;
			return this->chunk.get();// createPointer();
		}
		return nullptr;
	};
	DealocResult dealloc(uint8_t* ptr) override
	{
		if (this->chunk.get() == ptr)// getAddress(ptr))
		{
			if (this->useCount != 1)
				return DealocResult::DEALOC_NOT_ALLOCATED;
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
		assert(false);
		return this->memorysize;
	}
};

