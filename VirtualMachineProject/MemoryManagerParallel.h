#pragma once
#include <utility>
#include <map>
#include <array>
#include <mutex>
#include <list>
#include <thread>
#include <cassert>
#include <atomic>


#include "MemoryRange.h"
#include "MemoryChunkBase.h"
#include "MemoryPoolAllocatorInterface.h"
#include "MemoryChunkPrecise.h"
#include "MemoryChunkVerySmall.h"
#include "MemoryChunkAnySize.h"
#include "MemoryChunkPoolAllocator.h"
#include "MemorySizeRanges.h"




constexpr size_t MEMORY_BUCKET_SIZE_BITS = 15;
constexpr size_t MEMORY_BUCKET_COUNT_BITS = 7;
constexpr size_t MEMORY_ALLOC_CONTEXT_COUNT_BITS = 7;

constexpr size_t MEMORY_BUCKET_SIZE =
		size_t(1) << MEMORY_BUCKET_SIZE_BITS;

constexpr size_t MEMORY_BUCKET_COUNT = 
		size_t(1) << MEMORY_BUCKET_COUNT_BITS;

constexpr size_t MEMORY_ALLOC_CONTEXT_COUNT = 
		size_t(1) << MEMORY_ALLOC_CONTEXT_COUNT_BITS;



template<class PoolAllocatorType, class InternalStructAllocatorType, class LockType>
class MemoryManagerParallel
{
	using MemoryAddressRange = MemoryRange; 
	using MemorySizeRange = MemoryRange;


	//contains all memory chunks, indexed by address range
	std::array<std::pair<LockType, std::map<MemoryAddressRange, std::unique_ptr<MemoryChunkBase<PoolAllocatorType> > > >, MEMORY_BUCKET_COUNT> memory;

	//contains not full chunks, binded to context, indexed by chunksize
	std::array<std::pair<LockType, std::map<MemorySizeRange, std::list<MemoryChunkBase<PoolAllocatorType>*> > >, MEMORY_ALLOC_CONTEXT_COUNT> notfullchunks;

	//contains not full chunks, not binded to any context, indexed by chunksize
	std::pair<LockType, std::map<MemorySizeRange, std::list<MemoryChunkBase<PoolAllocatorType> * > > > freeStore;

	std::atomic_size_t emptyBlocksSize = 0;
	std::atomic_size_t emptyBlocksCount = 0;

	LockType cleanMutex;
	std::atomic_size_t cleanCounter = 0;

	PoolAllocatorType memoryPool;
	InternalStructAllocatorType internalAlloc;




	static constexpr int memorysize = sizeof(memory);
	static constexpr int notfullchunkssize = sizeof(notfullchunks);

	static size_t getAllocContext(size_t contextID = size_t(-1)) noexcept
	{
		if (contextID == size_t(-1))
		{
			auto thid = std::this_thread::get_id();
			std::hash<std::thread::id> hasher;
			contextID = hasher(thid);
		}
		return contextID % MEMORY_ALLOC_CONTEXT_COUNT;
	}
	static constexpr size_t getMemoryBucket(const uint8_t* ptr) noexcept
	{
		return (reinterpret_cast<size_t>(ptr) >> MEMORY_BUCKET_SIZE_BITS) & (MEMORY_BUCKET_COUNT - 1);
	}
	static size_t getNextAllocContext(size_t contextID = size_t(-1)) noexcept
	{
		if (contextID == size_t(-1))
		{
			auto thid = std::this_thread::get_id();
			std::hash<std::thread::id> hasher;
			contextID = hasher(thid);
		}
		return (contextID+1999) % MEMORY_ALLOC_CONTEXT_COUNT;
	}
	std::unique_ptr<MemoryChunkBase<PoolAllocatorType>> createNewChunk(size_t size, size_t bucketSize, MemorySizeRange sizebucket) 
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
		return uptr;
	}

public:

	MemoryManagerParallel() = default;
	MemoryManagerParallel(const PoolAllocatorType &pool): memoryPool(pool) { }
	MemoryManagerParallel(PoolAllocatorType&& pool) : memoryPool(std::move(pool)) { }

	void clean(bool force = false, size_t contextID = size_t(-1))
	{
		if (!cleanMutex.try_lock())
			return;

		const std::lock_guard<LockType> lockClean(cleanMutex, std::adopt_lock_t());//lock clean

		if (((emptyBlocksSize >= 1024 * 1024 * 16) && (emptyBlocksCount >= 8))
			|| (emptyBlocksSize >= 1024 * 1024 * 64)
			|| (emptyBlocksCount >= 256)
			|| (force == true))
		{
			++cleanCounter;
			if (cleanCounter % 64 == 0) force = true;
			bool cleanContexts = (cleanCounter % 8 == 0) | force;
			bool cleanFreeStore = true;
			
			
			
			auto& fsmutex = freeStore.first;

			if(cleanContexts)
			for (auto& context : notfullchunks)
			{
				auto& cmutex = context.first;
				const std::lock_guard<LockType> lockContext(cmutex);//lock context
				for (auto& sizeRange : context.second)
				{

					if (sizeRange.second.begin() != sizeRange.second.end())
					{
						const std::lock_guard<LockType> lockFreeStore(fsmutex);//lock free store
						for (auto chunkit = sizeRange.second.begin(); chunkit != sizeRange.second.end(); )
						{
							if ((*chunkit)->empty() || force)
							{
								emptyBlocksSize -= (*chunkit)->getMemorySize();
								emptyBlocksCount -= 1;
								auto ptr = (*chunkit);
								MemoryAddressRange mrange = { ptr->getAddresAsInt(), ptr->getAddresAsInt()+ ptr->getMemorySize()-1 };
								freeStore.second[sizeRange.first].push_back(ptr);
								chunkit = sizeRange.second.erase(chunkit);
							}
							else
								chunkit++;
						}
					}//unlock free store
				}
			}
			
			if (cleanFreeStore)
			{
				constexpr size_t toDeleteListSize = 128;
				std::list<MemoryChunkBase<PoolAllocatorType>* > toDeleteList;
				{
					const std::lock_guard<LockType> lockBucket(fsmutex);//lock free store
					for (auto& e : freeStore.second)
					{
						for (auto ee = e.second.begin(); ee != e.second.end(); )
						{
							if ((*ee)->empty())
							{
								emptyBlocksSize -= (*ee)->getMemorySize();
								emptyBlocksCount -= 1;
								auto tmpee = ee;
								++tmpee;
								toDeleteList.splice(toDeleteList.begin(), e.second, ee);// = { e.first, *ee };
								ee = tmpee;
							}
							else
								++ee;
						}
					}
				}//unlock free store

				for (auto &e: toDeleteList)
				{
					MemoryRange mrange = { (*e).getAddresAsInt(), (*e).getAddresAsInt() };
					auto bucketID = getMemoryBucket(reinterpret_cast<uint8_t*>(mrange.minv));
					auto& bucket = memory[bucketID];
					auto& bmutex = bucket.first;
					{
						const std::lock_guard<LockType> lockBucket(bmutex);//lock bucket
						auto it = bucket.second.find(mrange);
						if (it == bucket.second.end())
							assert(false);
						else
							bucket.second.erase(it);
					}//unlock bucket
				}
			}
		}

		if (force)
		{
			memoryPool.clean(0, 0);
		}
	
	}//unlock clean

	uint8_t* alloc(size_t size, size_t contextID = size_t(-1)) {
		
		auto SizeBucket = getSizeRange(size);//elementSize_perChunk_Map.find(MemoryRange{ size,size });
		//if (SizeBucketIt == elementSize_perChunk_Map.end())
		//	return nullptr;
		MemorySizeRange sizebucket = SizeBucket.first;
		size_t bucketSize = SizeBucket.second;

		auto context = getAllocContext(contextID);
		auto contexttmp = context;

		size_t counter = 3;
		while (counter) {
			auto& cmutexptr = notfullchunks[contexttmp].first;

			if (cmutexptr.try_lock())
			{
				context = contexttmp;
				break;
			}
			contexttmp = getNextAllocContext(contexttmp);
			--counter;
		}

		if (counter == 0)
		{
			auto* cmutexptr = &notfullchunks[context].first;
			cmutexptr->lock();
		}

		{
			auto& cmutex = notfullchunks[context].first;
			auto& notfull = notfullchunks[context].second;

			const std::lock_guard<LockType> lockContext(cmutex, std::adopt_lock_t());//lock context

			auto notusedIt = notfull.find(sizebucket);
			if (notusedIt != notfull.end() && !notusedIt->second.empty())
			{
				auto* chunk = notusedIt->second.front();

				auto bucketID = getMemoryBucket(reinterpret_cast<uint8_t*>(chunk->getAddresAsInt()));
				auto& bmutex = memory[bucketID].first;

				{
					const std::lock_guard<LockType> lockBucket(bmutex);//lock bucket

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
				}//unlock bucket
			}
			else
			{
				std::unique_ptr<MemoryChunkBase<PoolAllocatorType>> uptr;
				MemoryChunkBase<PoolAllocatorType>* rptr = nullptr;

				auto& fsmutex = freeStore.first;
				{
					const std::lock_guard<LockType> lockFreeStore(fsmutex);//lock free store
					auto& fstore = freeStore.second;
					auto fsit = fstore.find(sizebucket);
					if (fsit != fstore.end() && !fsit->second.empty())
					{
						rptr = fsit->second.front();
						fsit->second.pop_front();
					}
				}//unlock free store

				if (!rptr)
				{
					uptr = createNewChunk(size, bucketSize, sizebucket);
				}

				if (uptr || rptr)
				{
					MemoryAddressRange memrange;
					if(uptr)
						memrange = MemoryAddressRange{
							uptr->getAddresAsInt(),
							uptr->getAddresAsInt() + uptr->getMemorySize() - 1 };
					else
						memrange = MemoryAddressRange{
							rptr->getAddresAsInt(),
							rptr->getAddresAsInt() + rptr->getMemorySize() - 1 };

					auto bucketID = getMemoryBucket(reinterpret_cast<uint8_t*>(memrange.minv));
					auto& bmutex = memory[bucketID].first;

					uint8_t* ptr = nullptr;
					MemoryChunkBase<PoolAllocatorType> * chunkaddress = nullptr;
					if(uptr)
						chunkaddress = uptr.get();
					else
						chunkaddress = rptr;

					{
						const std::lock_guard<LockType> lockBucket(bmutex);//lock bucket
						
						if (uptr)
							ptr = uptr->alloc(size);
						else
							ptr = rptr->alloc(size);

						if (ptr == nullptr)
						{
							assert(false);
							throw std::bad_alloc();
						}

						if (uptr)
						{
							if (!uptr->full())
								notfull[sizebucket].push_front(chunkaddress);

							assert(memory[bucketID].second.count(memrange) == 0);
							memory[bucketID].second[memrange] = std::move(uptr);
						}
						else
						{
							if (!rptr->full())
								notfull[sizebucket].push_front(chunkaddress);

							if (rptr->empty())
							{
								emptyBlocksSize -= rptr->getMemorySize();
								emptyBlocksCount -= 1;
							}
						}
					}//unlock bucket
					return ptr;
				}
			}

		}//unlock context

		throw std::bad_alloc();
		return nullptr;
	}

	void dealloc(uint8_t* ptr, size_t contextID = size_t(-1)) 
	{
		if (ptr == nullptr)
			return;

		auto context = getAllocContext(contextID);
		auto bucketID = getMemoryBucket(ptr);
		size_t iteration = 0;
		while (iteration < MEMORY_BUCKET_COUNT) 
		{
			auto& bmutex = memory[bucketID].first;
			auto& bucket = memory[bucketID].second;
			{
				const std::lock_guard<LockType> lockBucket(bmutex);//lock bucket

				auto chunkIt = bucket.find(
					MemoryRange{
						reinterpret_cast<size_t>(ptr),
						reinterpret_cast<size_t>(ptr) });
				if (chunkIt == bucket.end())
				{
					iteration++;
					bucketID = ((bucketID == 0) ? MEMORY_BUCKET_COUNT : bucketID) - 1;
					continue;
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
				}
				else if (dealocFromChunkResult == DealocResult::WAS_FULL_CHUNK)
				{
					auto elemsize = chunkIt->second->elSize();
					auto bucketRR = getSizeRange(elemsize);

					auto& fsmutex = freeStore.first;
					{
						const std::lock_guard<LockType> lockFreeStore(fsmutex);//lock free store
						freeStore.second[bucketRR.first].push_back(chunkIt->second.get());
					}//unlock free store

					return;
				}
				else if (dealocFromChunkResult == DealocResult::WAS_FULL_IS_EMPTY)
				{
					auto elemsize = chunkIt->second->elSize();
					auto bucketRR = getSizeRange(elemsize); 

					auto& fsmutex = freeStore.first;
					{
						const std::lock_guard<LockType> lockFreeStore(fsmutex);//lock free store
						freeStore.second[bucketRR.first].push_back(chunkIt->second.get());
					}//unlock free store

					emptyBlocksSize += chunkIt->second->getMemorySize();
					emptyBlocksCount += 1;
				}
				else
				{
					assert(false);
					return;
				}
			}//unlock bucket

			clean();
			return;
		} 
		
		assert(false);
		return;
	}

};
