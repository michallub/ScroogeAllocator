#pragma once
#include <limits>
#include <new>

#include "MemoryAllocatorBase.h"
//#include "MemoryCustomAllocator.h"


/// void* allocators:

class MemoryAllocatorLight
{
	friend bool operator==(const MemoryAllocatorLight& A, const MemoryAllocatorLight& B) noexcept;
	friend bool operator!=(const MemoryAllocatorLight& A, const MemoryAllocatorLight& B) noexcept;

	MemoryAllocatorBase& allocator;

public:
	MemoryAllocatorLight(MemoryAllocatorBase& allocator) noexcept :allocator(allocator) {}
	
	void* allocate(size_t size)
	{
		if (auto p = allocator.allocate(size))
			return p;

		throw std::bad_alloc();
	}

	void deallocate(void* ptr, size_t size) noexcept
	{
		allocator.deallocate(ptr, size);
	}
};

inline bool operator==(const MemoryAllocatorLight& A, const MemoryAllocatorLight& B) noexcept  {
	return &A.allocator == &B.allocator;
}

inline bool operator!=(const MemoryAllocatorLight& A, const MemoryAllocatorLight& B) noexcept {
	return &A.allocator != &B.allocator;
}


/// typed allocators:


template <typename T1>
class MemoryAllocatorLightTyped
{
	template <typename T1, typename T2>
	friend bool operator==(const MemoryAllocatorLightTyped<T1>& A, const MemoryAllocatorLightTyped<T2>& B) noexcept;
	template <typename T1, typename T2>
	friend bool operator!=(const MemoryAllocatorLightTyped<T1>& A, const MemoryAllocatorLightTyped<T2>& B) noexcept;

	template <typename T2>
	friend class MemoryAllocatorLightTyped;

	MemoryAllocatorBase& allocator;

public:

	typedef T1 value_type;

	MemoryAllocatorLightTyped(MemoryAllocatorBase& allocator) noexcept :allocator(allocator) {}

	template <typename T2>
	MemoryAllocatorLightTyped(MemoryAllocatorLightTyped<T2>& other) noexcept :
		allocator(other.allocator) {
	}

	template <typename T2>
	MemoryAllocatorLightTyped(MemoryAllocatorLightTyped<T2>&& other) noexcept :
		allocator(other.allocator) {
	}

	template <typename T2>
	MemoryAllocatorLightTyped(const MemoryAllocatorLightTyped<T2>& other) noexcept :
		allocator(other.allocator) {
	}

	template <typename T2>
	MemoryAllocatorLightTyped(const MemoryAllocatorLightTyped<T2>&& other) noexcept :
		allocator(other.allocator) {
	}

	T1* allocate(size_t n)
	{
		if (n > (std::numeric_limits<std::size_t>::max)() / sizeof(T1))
			throw std::bad_alloc();

		if (auto p = static_cast<T1*>(allocator.allocate(n * sizeof(T1))))
			return p;

		throw std::bad_alloc();
	}

	void deallocate(T1* ptr, size_t n) noexcept
	{
		allocator.deallocate(ptr, sizeof(T1)*n);
	}
};

template <typename T1, typename T2>
inline bool operator==(const MemoryAllocatorLightTyped<T1>& A, const MemoryAllocatorLightTyped<T2>& B) noexcept  {
	return &A.allocator == &B.allocator;
}

template <typename T1, typename T2>
inline bool operator!=(const MemoryAllocatorLightTyped<T1>& A, const MemoryAllocatorLightTyped<T2>& B) noexcept  {
	return &A.allocator != &B.allocator;
}





