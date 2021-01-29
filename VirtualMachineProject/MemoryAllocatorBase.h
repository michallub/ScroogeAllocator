#pragma once
#include <iostream>
class MemoryAllocatorBase
{
private:

	virtual void* virtualallocate(size_t size) = 0;
	virtual void virtualdeallocate(void* ptr, size_t size) noexcept = 0;

public:
	virtual ~MemoryAllocatorBase() noexcept = default;
	
	void* allocate(size_t size) {
//		std::cout << "MemoryAllocatorBase::allocate(" << size << ")\n";
		return this->virtualallocate(size);
	};
	
	void deallocate(void* ptr, size_t size) noexcept {
		this->virtualdeallocate(ptr, size);
	};
};

