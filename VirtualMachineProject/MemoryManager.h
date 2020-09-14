#pragma once
#include <utility>
#include <map>
#include <memory>
#include <list>
#include <cassert>

#include "MemoryChunkBase.h"
#include "MemoryChunkPrecise.h"
#include "MemoryChunkVerySmall.h"
#include "MemoryChunkAnySize.h"
#include "MemoryRange.h"
#include "MemorySizeRanges.h"

#include "MemoryChunkPoolAllocator.h"
#include "MemoryPoolAllocatorInterface.h"



template<class PoolAllocatorType>
class MemoryManager
{
	using MemoryAddressRange = MemoryRange;
	using MemorySizeRange = MemoryRange;

/*
	constexpr static std::pair< MemoryRange, size_t> elementSize_perChunk_Table[] = {
		{{    1,     1}, 4096},
		{{    2,     2}, 4096},
		{{    3,     3}, 4096},
		{{    4,     4}, 4096},
		{{    5,     5}, 4096},
		{{    6,     6}, 4096},
		{{    7,     7}, 4096},
		{{    8,     8}, 4096},
		{{    9,     9}, 4096},
		{{   10,    10}, 4096},
		{{   11,    11}, 4096},
		{{   12,    12}, 4096},
		{{   13,    13}, 4096},
		{{   14,    14}, 4096},
		{{   15,    15}, 4096},
		{{   16,    16}, 4096},
		{{   17,    18}, 2048},
		{{   19,    20}, 2048},
		{{   21,    22}, 2048},
		{{   23,    24}, 2048},
		{{   25,    26}, 2048},
		{{   27,    28}, 2048},
		{{   29,    30}, 2048},
		{{   31,    32}, 2048},
		{{   33,    36}, 1024},
		{{   37,    40}, 1024},
		{{   41,    44}, 1024},
		{{   45,    48}, 1024},
		{{   49,    52}, 1024},
		{{   53,    56}, 1024},
		{{   57,    60}, 1024},
		{{   61,    64}, 1024},
		{{   65,    72},  512},
		{{   73,    80},  512},
		{{   81,    88},  512},
		{{   89,    96},  512},
		{{   97,   104},  512},
		{{  105,   112},  512},
		{{  113,   120},  512},
		{{  121,   128},  512},
		{{  129,   144},  256},
		{{  145,   160},  256},
		{{  161,   176},  256},
		{{  177,   192},  256},
		{{  193,   208},  256},
		{{  209,   224},  256},
		{{  225,   240},  256},
		{{  241,   256},  256},
		
		{{  257,   272},  128},
		{{  273,   288},  128},
		{{  289,   304},  128},
		{{  305,   320},  128},
		{{  321,   336},  128},
		{{  337,   352},  128},
		{{  353,   368},  128},
		{{  369,   384},  128},
		{{  385,   400},  128},
		{{  401,   416},  128},
		{{  417,   432},  128},
		{{  433,   448},  128},
		{{  449,   464},  128},
		{{  465,   480},  128},
		{{  481,   496},  128},
		{{  497,   512},  128},

		//{{  257,   288},  128},////////////////
		//{{  289,   320},  128},
		//{{  321,   352},  128},
		//{{  353,   384},  128},
		//{{  385,   416},  128},
		//{{  417,   448},  128},
		//{{  449,   480},  128},
		//{{  481,   512},  128},////////////////

		{{  513,   576},   64},
		{{  577,   640},   64},
		{{  641,   704},   64},
		{{  705,   768},   64},
		{{  769,   832},   64},
		{{  833,   896},   64},
		{{  897,   960},   64},
		{{  961,  1024},   64},
		{{ 1025,  1152},   32},
		{{ 1153,  1280},   32},
		{{ 1281,  1408},   32},
		{{ 1409,  1536},   32},
		{{ 1537,  1664},   32},
		{{ 1665,  1792},   32},
		{{ 1793,  1920},   32},
		{{ 1921,  2048},   32},
		{{ 2049,  2304},   16},
		{{ 2305,  2560},   16},
		{{ 2561,  2816},   16},
		{{ 2817,  3072},   16},
		{{ 3073,  3328},   16},
		{{ 3329,  3584},   16},
		{{ 3585,  3840},   16},
		{{ 3841,  4096},   16},
		{{ 4097,  4608},    8},
		{{ 4609,  5120},    8},
		{{ 5121,  5632},    8},
		{{ 5633,  6144},    8},
		{{ 6145,  6656},    8},
		{{ 6657,  7168},    8},
		{{ 7169,  7680},    8},
		{{ 7681,  8192},    8},
		{{ 8193,  9216},    4},
		{{ 9217, 10240},    4},
		{{10241, 11264},    4},
		{{11265, 12288},    4},
		{{12289, 13312},    4},
		{{13313, 14336},    4},
		{{14337, 15360},    4},
		{{15361, 16384},    4},
		{{16385, size_t(-1)},    1}
	};
	static std::map<MemoryRange, size_t > elementSize_perChunk_Map;
	static void staticinit() {
		if (elementSize_perChunk_Map.empty())
		{
			for (auto e : elementSize_perChunk_Table)
				elementSize_perChunk_Map.insert(e);
		}
	}
*/
	std::map<MemoryAddressRange, std::unique_ptr<MemoryChunkBase<PoolAllocatorType>>> memory;
	std::map<MemorySizeRange, std::list<MemoryChunkBase<PoolAllocatorType>*>> notfullchunks;
	size_t emptyBlocksSize = 0;
	size_t emptyBlocksCount = 0;

	PoolAllocatorType memoryPool;
public:
	void clean(bool force = false) {
		if (((emptyBlocksSize >= 1024 * 1024 * 16) && (emptyBlocksCount >= 8))
			|| (emptyBlocksSize >= 1024 * 1024 * 256)
			|| (emptyBlocksCount >= 256)
			|| (force == true))
		{
			//			size_t copybefore = emptyBlocksSize;
			//			size_t deletecounter = 0;
			for (auto& e : notfullchunks) {
				auto endddddd = e.second.end();
				for (auto ee = e.second.begin(); ee != e.second.end(); )
				{
					if ((*ee)->empty())
					{
						emptyBlocksSize -= (*ee)->getMemorySize();
						emptyBlocksCount -= 1;
						MemoryRange mrange = { (*ee)->getAddresAsInt(), (*ee)->getAddresAsInt() };
						auto it = memory.find(mrange);
						if (it == memory.end())
							assert(false);
						ee = e.second.erase(ee);

						memory.erase(it);
						//assert(memoryPool.correctnessTest());
						//						deletecounter++;
					}
					else
						ee++;
				}
			}

			if (force)
			{
				memoryPool.clean(0,0);
			}
			//			size_t copyafter = emptyBlocksSize;
			//			std::cout << copybefore << " " << copyafter << " " << (copybefore - copyafter) << " " << deletecounter << "\n";
		}
	}

	MemoryManager() {
		//staticinit();
	}

	uint8_t* alloc(size_t size) {
		auto SizeBucket = getSizeRange(size); 
		MemorySizeRange sizebucket = SizeBucket.first;
		size_t bucketSize = SizeBucket.second;
		auto notusedIt = notfullchunks.find(sizebucket);
		if (notusedIt != notfullchunks.end() && !notusedIt->second.empty())
		{

			auto* chunk = notusedIt->second.front();
			if (chunk->empty())
			{
				emptyBlocksSize -= chunk->getMemorySize();
				emptyBlocksCount -= 1;
			}
			auto ptr = chunk->alloc(size);
			if (ptr == nullptr)
			{
				assert(false);
				throw std::bad_alloc();
			}
			if (chunk->full())
				notusedIt->second.pop_front();
			return ptr;
		}
		else
		{
			std::unique_ptr<MemoryChunkBase<PoolAllocatorType>> uptr;
			if (bucketSize == 4096)
			{
				MemoryChunkPrecise<PoolAllocatorType>* rawptr = 
					std::make_unique<MemoryChunkPrecise<PoolAllocatorType>>(
						&memoryPool, bucketSize * sizebucket.maxv, sizebucket.maxv, bucketSize).release();
				uptr.reset(static_cast<MemoryChunkBase<PoolAllocatorType>*>(rawptr));
			}
			else if (bucketSize == 2048 || bucketSize == 1024)
			{
				MemoryChunkVerySmall<PoolAllocatorType>* rawptr = 
					std::make_unique<MemoryChunkVerySmall<PoolAllocatorType>>(
						&memoryPool, bucketSize * sizebucket.maxv, sizebucket.maxv, bucketSize).release();
				uptr.reset(static_cast<MemoryChunkBase<PoolAllocatorType>*>(rawptr));
			}
			else if (bucketSize == 512 || bucketSize == 256 || bucketSize == 128)
			{
				MemoryChunkSmall<PoolAllocatorType>* rawptr = std::make_unique<MemoryChunkSmall<PoolAllocatorType>>(
					&memoryPool, bucketSize * sizebucket.maxv, sizebucket.maxv, bucketSize).release();
				uptr.reset(static_cast<MemoryChunkBase<PoolAllocatorType>*>(rawptr));
			}
			else if (bucketSize == 64 || bucketSize == 32 || bucketSize == 16)
			{
				MemoryChunkMedium<PoolAllocatorType>* rawptr = std::make_unique<MemoryChunkMedium<PoolAllocatorType>>(
					&memoryPool, bucketSize * sizebucket.maxv, sizebucket.maxv, bucketSize).release();
				uptr.reset(static_cast<MemoryChunkBase<PoolAllocatorType>*>(rawptr));
			}
			else if (bucketSize == 8 || bucketSize == 4)
			{
				MemoryChunkLarge<PoolAllocatorType>* rawptr = std::make_unique<MemoryChunkLarge<PoolAllocatorType>>(
					&memoryPool, bucketSize * sizebucket.maxv, sizebucket.maxv, bucketSize).release();
				uptr.reset(static_cast<MemoryChunkBase<PoolAllocatorType>*>(rawptr));
			}
			else if (bucketSize == 8 || bucketSize == 1)
			{
				MemoryChunkAnySize<PoolAllocatorType>* rawptr = std::make_unique<MemoryChunkAnySize<PoolAllocatorType>>(
					&memoryPool, bucketSize * size).release();
				uptr.reset(static_cast<MemoryChunkBase<PoolAllocatorType>*>(rawptr));
			}
			else
			{
				assert(false);
			}

			auto ptr = uptr->alloc(size);

			if (ptr == nullptr)
			{
				assert(false);
				throw std::bad_alloc();
			}
			if (!uptr->full())
				notfullchunks[sizebucket].push_front(uptr.get());
			
			auto memrange =
				MemoryRange{
					uptr->getAddresAsInt(),
					uptr->getAddresAsInt() + uptr->getMemorySize() - 1 };

			assert(memory.count(memrange) == 0);
			memory[memrange] = std::move(uptr);
			return ptr;
		}
	}
	void dealloc(uint8_t* ptr) {

		if (ptr == nullptr)
			return;

		auto chunkIt = memory.find(
			MemoryRange{
				reinterpret_cast<size_t>(ptr),
				reinterpret_cast<size_t>(ptr) });
		if (chunkIt == memory.end())
		{
			assert(false);
			return;
		}

		auto dealocFromChunkResult = chunkIt->second->dealloc(ptr);
		if (dealocFromChunkResult == DealocResult::OK)
		{
			return;
		}
		else if (dealocFromChunkResult == DealocResult::NOW_IS_EMPTY_CHUNK)
		{
			emptyBlocksSize += chunkIt->second->getMemorySize();
			emptyBlocksCount += 1;
			clean();

			return;
		}
		else if (dealocFromChunkResult == DealocResult::WAS_FULL_CHUNK)
		{
			auto elemsize = chunkIt->second->elSize();
			auto bucketRR = getSizeRange(elemsize);

			notfullchunks[bucketRR.first].push_front(chunkIt->second.get());
			return;
		}
		else if (dealocFromChunkResult == DealocResult::WAS_FULL_IS_EMPTY)
		{
			auto elemsize = chunkIt->second->elSize();
			auto bucketRR = getSizeRange(elemsize);

			notfullchunks[bucketRR.first].push_front(chunkIt->second.get());

			emptyBlocksSize += chunkIt->second->getMemorySize();
			emptyBlocksCount += 1;
			clean();

			return;
		}
		else
		{
			assert(false);
			return;
		}
	}

	//bool correctnessTest() const noexcept {
	//	return memoryPool.correctnessTest();
	//
	//}
};

//template<class PoolAllocatorType>
//std::map<MemoryRange, size_t > MemoryManager< PoolAllocatorType>::elementSize_perChunk_Map;










extern MemoryManager<MemoryPoolAllocatorInterface> globalMemoryManagerWithPoolAllocInterface;
//extern MemoryManager<MemoryChunkPoolAllocator> globalMemoryManagerWithChunkPoolAlloc;



template <typename T>
struct MemoryManagerAllocatorWithPoolAllocInterface
{
	typedef T value_type;
	MemoryManagerAllocatorWithPoolAllocInterface() = default;

	template <typename U>
	constexpr MemoryManagerAllocatorWithPoolAllocInterface(
		const MemoryManagerAllocatorWithPoolAllocInterface<U>&) noexcept {}

	T* allocate(std::size_t n) {
		if (n > (std::numeric_limits<int>::max)() / sizeof(T))
			throw std::bad_array_new_length();
		//std::cout << "MemoryManagerAllocatorWithPoolAllocInterface::allocate: " << n << " " << sizeof(T) << "\n";
		try {
			return (T*)globalMemoryManagerWithPoolAllocInterface.alloc(n * sizeof(T));
		}
		catch (...) {
		}

		throw std::bad_alloc();
	}

	void deallocate(T* ptr, std::size_t n = 0) noexcept {
		globalMemoryManagerWithPoolAllocInterface.dealloc((uint8_t*)ptr);
	}
};

template <typename T, typename U>
bool operator==(
	const MemoryManagerAllocatorWithPoolAllocInterface <T>&, 
	const MemoryManagerAllocatorWithPoolAllocInterface <U>&) { return true; }

template <typename T, typename U>
bool operator!=(
	const MemoryManagerAllocatorWithPoolAllocInterface <T>&, 
	const MemoryManagerAllocatorWithPoolAllocInterface <U>&) { return false; }



/*
template <typename T>
struct MemoryManagerAllocatorWithChunkPoolAlloc
{
	typedef T value_type;
	MemoryManagerAllocatorWithChunkPoolAlloc() = default;

	template <typename U>
	constexpr MemoryManagerAllocatorWithChunkPoolAlloc(
		const MemoryManagerAllocatorWithChunkPoolAlloc<U>&) noexcept {}

	T* allocate(std::size_t n) {
		if (n > (std::numeric_limits<int>::max)() / sizeof(T))
			throw std::bad_array_new_length();

		//std::cout << "MemoryManagerAllocatorWithChunkPoolAlloc::allocate: " << n << " " << sizeof(T) << "\n";
		try {
			return (T*)globalMemoryManagerWithChunkPoolAlloc.alloc(n * sizeof(T));
		}
		catch (...) {
		}

		throw std::bad_alloc();
	}

	void deallocate(T* ptr, std::size_t n = 0) noexcept {
		globalMemoryManagerWithChunkPoolAlloc.dealloc((uint8_t*)ptr);
	}
};

template <typename T, typename U> 
bool operator==(
	const MemoryManagerAllocatorWithChunkPoolAlloc <T>&, 
	const MemoryManagerAllocatorWithChunkPoolAlloc <U>&) { return true; }

template <typename T, typename U>
bool operator!=(
	const MemoryManagerAllocatorWithChunkPoolAlloc <T>&, 
	const MemoryManagerAllocatorWithChunkPoolAlloc <U>&) { return false; } 


	*/
