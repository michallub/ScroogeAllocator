#include "VMPointer.h"
#include "VMMemory.h"
inline size_t VMPointer::allocatedMemory() {
	assert(memchunk != nullptr);
	return memchunk->elSize();
}
