#pragma once

#include <Windows.h>
#include <memoryapi.h>
#include <utility>
#include <array>
#include <cmath>



class MemoryPoolAllocator
{
	struct PoolType{
		void* ptr;
		size_t buforSize;
		size_t elementSize;
		 

	};
	static constexpr std::array < size_t,35> POOL_SIZES = {
		4ull * 1024, 
		8ull * 1024, 
		16ull * 1024, 
		32ull * 1024,
		64ull * 1024, 
		128ull * 1024, 
		256ull * 1024, 
		512ull * 1024,
		1ull   * 1024 * 1024,
		2ull   * 1024 * 1024,
		4ull   * 1024 * 1024,
		8ull   * 1024 * 1024,
		16ull  * 1024 * 1024,
		32ull  * 1024 * 1024,
		64ull  * 1024 * 1024,
		128ull * 1024 * 1024,
		256ull * 1024 * 1024,
		512ull * 1024 * 1024,
		1ull   * 1024 * 1024 * 1024,
		2ull   * 1024 * 1024 * 1024,
		4ull   * 1024 * 1024 * 1024,
		8ull   * 1024 * 1024 * 1024,
		16ull  * 1024 * 1024 * 1024,
		32ull  * 1024 * 1024 * 1024,
		64ull  * 1024 * 1024 * 1024,
		128ull * 1024 * 1024 * 1024,
		256ull * 1024 * 1024 * 1024,
		512ull * 1024 * 1024 * 1024,
		1ull * 1024 * 1024 * 1024 * 1024,
		2ull * 1024 * 1024 * 1024 * 1024,
		4ull * 1024 * 1024 * 1024 * 1024,
		8ull * 1024 * 1024 * 1024 * 1024,
		16ull * 1024 * 1024 * 1024 * 1024,
		32ull * 1024 * 1024 * 1024 * 1024,
		64ull * 1024 * 1024 * 1024 * 1024
	};

	size_t memorySize;
	std::array<void*, POOL_SIZES.size()> pools;

	constexpr static size_t MemorySizeRoundUp(size_t size, const size_t granularity = 20) noexcept {
		const size_t gransize = size_t(1) << 20;
		const size_t granmask = gransize - 1;

		return ((size & granmask) == 0 ? size : (size & ~granmask) + gransize);
	}
	constexpr static size_t MemorySizeRoundDown(size_t size, const size_t granularity = 20) noexcept {
		const size_t gransize = size_t(1) << 20;
		const size_t granmask = gransize - 1;

		return  (size & ~granmask);
	}

public:
	MemoryPoolAllocator(size_t memorySize) :memorySize(memorySize) {
		const size_t memorySizeRounded = MemorySizeRoundUp(memorySize);
		const size_t memorySizeRoundedX2 = memorySizeRounded*2;

		for (int i = 0; i < pools.size(); i++)
		{
			const size_t curpoolsize = POOL_SIZES[i];



		}



	}
};

