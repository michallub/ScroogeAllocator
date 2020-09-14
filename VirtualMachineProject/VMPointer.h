#pragma once
#include <cassert>
//#include "VMMemory.h"


class VMMemoryChunkBase;

class VMPointer
{
	friend VMMemoryChunkBase;
	friend class VMMemory;

	void* ptr = 0;
	VMMemoryChunkBase* memchunk = 0;	
	VMPointer(void* ptr, VMMemoryChunkBase* memchunk) noexcept :
		ptr(ptr), memchunk(memchunk) { };
public:
	VMPointer() noexcept = default;
	bool isEmpty() {
		return ptr == nullptr;
	}
	size_t allocatedMemory();



private:


	

};

