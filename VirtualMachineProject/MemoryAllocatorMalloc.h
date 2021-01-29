#pragma once

#include "MemoryAllocatorBase.h"
#include <cstdlib>

class MemoryAllocatorMalloc : public MemoryAllocatorBase
{
private:
	void* virtualallocate(size_t size) override
	{
		return std::malloc(size);
	}

	void virtualdeallocate(void* ptr, size_t size) noexcept override
	{
		free(ptr);
	}

public:
	~MemoryAllocatorMalloc() noexcept override = default;

	
};

