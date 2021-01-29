#pragma once

#include <cstdint>


#include "MemoryPoolAllocatorInterface.h"
#include "MemoryAllocatorBase.h"
#include "MemoryAllocatorLight.h"


class MemoryCustomAllocator
{
	MemoryAllocatorBase &poolAlloc;
	MemoryAllocatorBase &internalAlloc;
public:
	MemoryCustomAllocator(MemoryAllocatorBase& poolAlloc, MemoryAllocatorBase& internalAlloc) noexcept:
		poolAlloc(poolAlloc), internalAlloc(internalAlloc) {}

	void* allocPoolMemory(size_t size) {
		return poolAlloc.allocate(size);
	}
	void deallocPoolMemory(void* ptr, size_t size) {
		poolAlloc.deallocate(ptr, size);
	}
	void* allocInternalStructMemory(size_t size) {
		return internalAlloc.allocate(size);
	}
	void deallocInternalStructMemory(void* ptr, size_t size) {
		internalAlloc.deallocate(ptr, size);
	}

	MemoryAllocatorLight getPoolAllocatorLight()
	{
		return MemoryAllocatorLight(poolAlloc);
	}
	MemoryAllocatorLight getInternalAllocatorLight()
	{
		return MemoryAllocatorLight(internalAlloc);
	}

	template <typename T1>
	MemoryAllocatorLightTyped<T1> getPoolAllocatorLightTyped()
	{
		return MemoryAllocatorLightTyped<T1>(poolAlloc);
	}

	template <typename T1>
	MemoryAllocatorLightTyped<T1> getInternalAllocatorLightTyped()
	{
		return MemoryAllocatorLightTyped<T1>(internalAlloc);
	}


};


class MemoryCustomAllocatorRef
{
	MemoryCustomAllocator& ref;
public:

	MemoryCustomAllocatorRef(MemoryCustomAllocator& ref) : ref(ref)
	{
	}

	void* allocPoolMemory(size_t size) {
		return ref.allocPoolMemory(size);
	}
	void deallocPoolMemory(void* ptr, size_t size) {
		ref.deallocPoolMemory(ptr, size);
	}
	void* allocInternalStructMemory(size_t size) {
		return ref.allocInternalStructMemory(size);
	}
	void deallocInternalStructMemory(void* ptr, size_t size) {
		ref.deallocInternalStructMemory(ptr, size);
	}

	MemoryAllocatorLight getPoolAllocatorLight()
	{
		return ref.getPoolAllocatorLight();
	}
	MemoryAllocatorLight getInternalAllocatorLight()
	{
		return ref.getInternalAllocatorLight();
	}

	template <typename T1>
	MemoryAllocatorLightTyped<T1> getPoolAllocatorLightTyped()
	{
		return ref.getPoolAllocatorLightTyped<T1>();
	}

	template <typename T1>
	MemoryAllocatorLightTyped<T1> getInternalAllocatorLightTyped()
	{
		return ref.getInternalAllocatorLightTyped<T1>();
	}
};

const size_t sizeallocs = sizeof(MemoryCustomAllocator);
const size_t sizeallocsref = sizeof(MemoryCustomAllocatorRef);
