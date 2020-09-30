#pragma once
//#include <cstdint>
#include <cstdlib>
class MemoryPoolAllocatorInterface
{
public:
	void* alloc(size_t size) noexcept
	{
		return (void*)std::malloc(size);
	}
	void dealloc(void* ptr) noexcept
	{
		return std::free(ptr);
	}
	void clean(size_t shouldBe = 8, size_t tolerance = 8) noexcept {}
};

