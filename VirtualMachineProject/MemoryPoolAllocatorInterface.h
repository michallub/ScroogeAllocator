#pragma once
#include <cstdint>
#include <cstdlib>
class MemoryPoolAllocatorInterface
{
public:
	uint8_t* alloc(size_t size) noexcept
	{
		return (uint8_t*)std::malloc(size);
	}
	void dealloc(uint8_t* ptr) noexcept
	{
		return std::free(ptr);
	}
	void clean(size_t shouldBe = 8, size_t tolerance = 8) noexcept {}
};

