#pragma once
#include <utility>
#include <map>
#include <array>
#include <mutex>
#include <list>
#include <thread>
#include <cassert>
#include <atomic>
#include <limits>
#include <exception>


#include "MemoryRange.h"
#include "MemoryChunkBase.h"
#include "MemoryPoolAllocatorInterface.h"
#include "MemoryChunkPrecise.h"
#include "MemoryChunkVerySmall.h"
#include "MemoryChunkAnySize.h"
#include "MemoryChunkPoolAllocator.h"
#include "MemorySizeRanges.h"
#include "MemoryCustomAllocator.h"
#include "MemoryBucket.h"


constexpr size_t MEMORY_BUCKET_SIZE_BITS_DEFAULT = 16;
constexpr size_t MEMORY_BUCKET_COUNT_BITS_DEFAULT = 7;
constexpr size_t MEMORY_ALLOC_CONTEXT_COUNT_BITS_DEFAULT = 7;
constexpr size_t MEMORY_FREESTORE_COUNT_DEFAULT = 1;



class MemoryManagerArguments {
	MemoryAllocatorBase& internalAllocator; 
	MemoryAllocatorBase& poolAllocator;
	size_t BucketAddressRangeSize = MEMORY_BUCKET_SIZE_BITS_DEFAULT;
	size_t BucketCount = MEMORY_BUCKET_COUNT_BITS_DEFAULT;
	size_t AllocContextCount = MEMORY_ALLOC_CONTEXT_COUNT_BITS_DEFAULT;
	size_t FreeStroreCount = MEMORY_FREESTORE_COUNT_DEFAULT;

public:
	MemoryManagerArguments(MemoryAllocatorBase& internalAllocator, MemoryAllocatorBase& poolAllocator) :
		internalAllocator(internalAllocator), 
		poolAllocator(poolAllocator)
	{
	}
	MemoryManagerArguments(MemoryAllocatorBase& internalAndPoolAllocator) :
		internalAllocator(internalAndPoolAllocator), 
		poolAllocator(internalAndPoolAllocator)
	{
	}

	MemoryManagerArguments& setBucketAddressRangeSize(size_t bits) {
		assert(bits < 32);
		BucketAddressRangeSize = bits;
		return *this;
	}
	MemoryManagerArguments& setBucketCount(size_t bits) {
		assert(bits < 16);
		BucketCount = bits;
		return *this;
	}
	MemoryManagerArguments& setAllocContextCount(size_t bits) {
		assert(bits < 16);
		AllocContextCount = bits;
		return *this;
	}
	MemoryManagerArguments& setFreeStroreCount(size_t count) {
		assert(count < 65536);
		FreeStroreCount = count;
		return *this;
	}

	MemoryAllocatorBase& getInternalAllocator() {
		return internalAllocator;
	}
	MemoryAllocatorBase& getPoolAllocator() {
		return poolAllocator;
	}
	size_t getBucketAddressRangeSize() {
		return BucketAddressRangeSize;
	}
	size_t& getBucketCount() {
		return BucketCount;
	}
	size_t& getAllocContextCount() {
		return AllocContextCount;
	}
	size_t& getFreeStroreCount() {
		return FreeStroreCount;
	}
};



struct MemoryAllocationInfo: public MemoryMemInfo
{
	MemoryAllocationInfo(MemoryMemInfo meminfo = MemoryMemInfo()) :MemoryMemInfo(meminfo) {}
};



template<class LockType>
class MemoryManagerParallel
{
	using MemoryManagerType = MemoryManagerParallel<LockType>;
	using MemoryAllocatorsType = MemoryCustomAllocator;

	using MemoryAddressRange = MemoryRange; 
	using MemorySizeRange = MemoryRange;
	using MemoryBaseChunkType = MemoryChunkBase;
	using DeleterBaseChunkType = ChunkDeleterT<MemoryBaseChunkType>;

	using MemoryUniquePointerType = std::unique_ptr<MemoryBaseChunkType, DeleterBaseChunkType>;
	using MemoryMapType = std::map< MemoryAddressRange, MemoryUniquePointerType, std::less<MemoryAddressRange>, MemoryAllocatorLightTyped< std::pair<const MemoryAddressRange, MemoryUniquePointerType> > >;
	using MemoryBucketType = MemoryBucket<LockType>;
	using DeleterMemoryBucketType = ChunkDeleterT<MemoryBucketType>;
	using MemoryBucketUniquePtrType = std::unique_ptr<MemoryBucketType, DeleterMemoryBucketType>;
	using MemoryBucketArrayType = std::vector<MemoryBucketUniquePtrType, MemoryAllocatorLightTyped<MemoryBucketUniquePtrType>>;
	using ChunksRawPointerType = MemoryBaseChunkType*;
	using ContextType = MemoryContext<LockType>;
	using DeleterContextType = ChunkDeleterT<ContextType>;
	using ContextUniquePtrType = std::unique_ptr<ContextType, DeleterContextType>;
	using ContextArrayType = std::vector<ContextUniquePtrType, MemoryAllocatorLightTyped<ContextUniquePtrType>>;//
	using ContextFreeStoreArrayType = ContextArrayType;
	MemoryCustomAllocator allocators;

	//contains all memory chunks, indexed by address range
	MemoryBucketArrayType memory;

	//contains not full chunks, binded to context, indexed by chunksize
	ContextArrayType notfullchunks;

	//contains not full chunks, not binded to any context, indexed by chunksize
	ContextFreeStoreArrayType freeStore;

	std::atomic_size_t emptyBlocksSize = 0;
	std::atomic_size_t emptyBlocksCount = 0;

	LockType cleanMutex;
	std::atomic_size_t cleanCounter = 0;

	size_t BucketAddressRangeSizeBits = 0;
	size_t BucketAddressRangeSize = 0;
	size_t BucketCountBits = 0;
	size_t BucketCount = 0;
	size_t AllocContextCountBits = 0;
	size_t AllocContextCount = 0;
	size_t FreeStroreCount = 0;

	static constexpr int memorysize = sizeof(memory);
	static constexpr int notfullchunkssize = sizeof(notfullchunks);

	size_t getAllocContext(size_t contextID = size_t(-1)) const noexcept
	{
		if (contextID == size_t(-1))
		{
			auto thid = std::this_thread::get_id();
			std::hash<std::thread::id> hasher;
			contextID = hasher(thid);
		}
		return contextID % AllocContextCount;
	}
	size_t getMemoryBucket(const uint8_t* ptr) const noexcept
	{
		return (reinterpret_cast<size_t>(ptr) >> BucketAddressRangeSizeBits) & (BucketCount - 1);
	}
	size_t getNextAllocContext(size_t contextID = size_t(-1)) const noexcept
	{
		if (contextID == size_t(-1))
		{
			auto thid = std::this_thread::get_id();
			std::hash<std::thread::id> hasher;
			contextID = hasher(thid);
		}
		return (contextID+1999) % AllocContextCount;
	}
	MemoryUniquePointerType createNewChunk(MemorySizeRangeInfo msri, size_t size)
	{
		//MemoryUniquePointerType uptr = { }; //, DeleterType(internalAlloc)
		
		if (msri.chunkType == MemoryChunkType::Precise)
		{
			using ChunkType = MemoryChunkPrecise;
			auto* buf = allocators.allocInternalStructMemory(sizeof(ChunkType));
			ChunkType* rawptr = new (buf) ChunkType(
				allocators, msri.buforSize, msri.range.maxv, msri.elPerChunk);
			assert((void*)buf == (void*)rawptr);
			return MemoryUniquePointerType(static_cast<MemoryBaseChunkType*>(rawptr));
			//uptr.reset(static_cast<MemoryBaseChunkType*>(rawptr));
		}
		else if (msri.chunkType == MemoryChunkType::VerySmall)
		{
			using ChunkType = MemoryChunkVerySmall;
			auto* buf = allocators.allocInternalStructMemory(sizeof(ChunkType));
			ChunkType* rawptr = new (buf) ChunkType(
				allocators, msri.buforSize, msri.range.maxv, msri.elPerChunk);
			assert((void*)buf == (void*)rawptr);
			return MemoryUniquePointerType(static_cast<MemoryBaseChunkType*>(rawptr));
			//uptr.reset(static_cast<MemoryBaseChunkType*>(rawptr));
		}
		else if (msri.chunkType == MemoryChunkType::Small)
		{
			using ChunkType = MemoryChunkSmall;
			auto* buf = allocators.allocInternalStructMemory(sizeof(ChunkType));
			ChunkType* rawptr = new (buf) ChunkType(
				allocators, msri.buforSize, msri.range.maxv, msri.elPerChunk);
			assert((void*)buf == (void*)rawptr);
			return MemoryUniquePointerType(static_cast<MemoryBaseChunkType*>(rawptr));
			//uptr.reset(static_cast<MemoryBaseChunkType*>(rawptr));
		}
		else if (msri.chunkType == MemoryChunkType::Medium)
		{
			using ChunkType = MemoryChunkMedium;
			auto* buf = allocators.allocInternalStructMemory(sizeof(ChunkType));
			ChunkType* rawptr = new (buf) ChunkType(
				allocators, msri.buforSize, msri.range.maxv, msri.elPerChunk);
			assert((void*)buf == (void*)rawptr);
			return MemoryUniquePointerType(static_cast<MemoryBaseChunkType*>(rawptr));
			//uptr.reset(static_cast<MemoryBaseChunkType*>(rawptr));
		}
		else if (msri.chunkType == MemoryChunkType::Large)
		{
			using ChunkType = MemoryChunkLarge;
			auto* buf = allocators.allocInternalStructMemory(sizeof(ChunkType));
			ChunkType* rawptr = new (buf) ChunkType(
				allocators, msri.buforSize, msri.range.maxv, msri.elPerChunk);
			assert((void*)buf == (void*)rawptr);
			return MemoryUniquePointerType(static_cast<MemoryBaseChunkType*>(rawptr));
			//uptr.reset(static_cast<MemoryBaseChunkType*>(rawptr));
		}
		else if (msri.chunkType == MemoryChunkType::AnySize)
		{
			using ChunkType = MemoryChunkAnySize;
			auto* buf = allocators.allocInternalStructMemory(sizeof(ChunkType));
			ChunkType* rawptr = new (buf) ChunkType(
				allocators, size);
			assert((void*)buf == (void*)rawptr);
			return MemoryUniquePointerType(static_cast<MemoryBaseChunkType*>(rawptr));
			//uptr.reset(static_cast<MemoryBaseChunkType*>(rawptr));
		}
		else
		{
			assert(false);
		}
		return MemoryUniquePointerType();
		//return uptr;
	}

	size_t getFreeStoreContext(size_t contextID = size_t(-1)) const noexcept
	{
		if (contextID == size_t(-1))
		{
			auto thid = std::this_thread::get_id();
			std::hash<std::thread::id> hasher;
			contextID = hasher(thid);
		}
		return (contextID + 1999) % FreeStroreCount;
	}

	bool initBuckets(size_t bucketAddressRangeSizeBits, size_t bucketCountBits) {
		if (BucketAddressRangeSizeBits != 0) return false;
		if (BucketCountBits != 0) return false;
		if (bucketAddressRangeSizeBits == 0) return false;
		if (bucketCountBits == 0) return false;
		if (bucketAddressRangeSizeBits > 32) return false;
		if (bucketCountBits > 16) return false;

		BucketAddressRangeSizeBits = bucketAddressRangeSizeBits;
		BucketCountBits = bucketCountBits;
		BucketAddressRangeSize = size_t(1) << bucketAddressRangeSizeBits;
		BucketCount = size_t(1) << BucketCountBits;

		memory.reserve(BucketCount);
		for (size_t i = 0; i < BucketCount; i++)
		{
			auto* tmpvoid = allocators.allocInternalStructMemory(sizeof(MemoryBucketType));
			auto* tmptyped = new (tmpvoid) MemoryBucketType(allocators);
			std::unique_ptr<MemoryBucketType, DeleterMemoryBucketType> uptr(tmptyped);
			memory.push_back(std::move(uptr));
		}

		return true;
	}

	bool initContexts(size_t allocContextCountBits)
	{
		if (AllocContextCountBits != 0) return false;
		if (allocContextCountBits == 0) return false;
		if (allocContextCountBits > 16) return false;
		
		AllocContextCountBits = allocContextCountBits;
		AllocContextCount = size_t(1) << AllocContextCountBits;

		notfullchunks.reserve(AllocContextCount);
		for (size_t i = 0; i < AllocContextCount; i++)
		{
			auto* tmpvoid = allocators.allocInternalStructMemory(sizeof(ContextType));
			auto* tmptyped = new (tmpvoid) ContextType(allocators);
			std::unique_ptr<ContextType, DeleterContextType> uptr(tmptyped);
			notfullchunks.push_back(std::move(uptr));
		}

		return true;
	}

	bool initFreeStore(size_t freeStroreCount)
	{
		if (FreeStroreCount != 0) return false;
		if (freeStroreCount == 0) return false;
		if (freeStroreCount > 65536) return false;

		FreeStroreCount = freeStroreCount;

		freeStore.reserve(FreeStroreCount);
		for (size_t i = 0; i < FreeStroreCount; i++)
		{
			auto* tmpvoid = allocators.allocInternalStructMemory(sizeof(ContextType));
			auto* tmptyped = new (tmpvoid) ContextType(allocators);
			std::unique_ptr<ContextType, DeleterContextType> uptr(tmptyped);
			freeStore.push_back(std::move(uptr));
		}

		return true;
	}

public:
	

	MemoryManagerParallel(MemoryManagerArguments args) :
		allocators(args.getPoolAllocator(), args.getInternalAllocator()),
		memory(allocators.getInternalAllocatorLightTyped<MemoryBucketUniquePtrType>()),
		notfullchunks(allocators.getInternalAllocatorLightTyped<ContextUniquePtrType>()),
		freeStore(allocators.getInternalAllocatorLightTyped<ContextUniquePtrType>())
	{
		initBuckets(args.getBucketAddressRangeSize(), args.getBucketCount());
		initContexts(args.getAllocContextCount());
		initFreeStore(args.getFreeStroreCount());
	}

	MemoryManagerParallel(const MemoryManagerParallel&) = delete;
	MemoryManagerParallel(MemoryManagerParallel&&) = delete;



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
			
			auto fsContext = getFreeStoreContext(contextID);
			auto& fs = freeStore[fsContext];

			auto& fsmutex = fs->getLock();

			if(cleanContexts)
			for (auto& context : notfullchunks)
			{
				auto& cmutex = context->getLock();
				const std::lock_guard<LockType> lockContext(cmutex);//lock context
				for (auto& sizeRange : context->getContext())
				{

					if (sizeRange.second.begin() != sizeRange.second.end())
					{
						const std::lock_guard<LockType> lockFreeStore(fsmutex);//lock free store
						for (auto chunkit = sizeRange.second.begin(); chunkit != sizeRange.second.end(); )
						{
							if ((*chunkit)->empty() || force)
							{
								auto ptr = (*chunkit);
								MemoryAddressRange mrange = { ptr->getAddresAsInt(), ptr->getAddresAsInt()+ ptr->getMemorySize()-1 };

								auto oldChunkIt = chunkit++;
								auto& dstlist = fs->getChunkList(sizeRange.first);
								auto& srclist = sizeRange.second;
								dstlist.splice(dstlist.begin(), srclist, oldChunkIt);
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
				std::list<MemoryBaseChunkType* , MemoryAllocatorLightTyped<MemoryBaseChunkType*>> toDeleteList(allocators.getInternalAllocatorLightTyped<MemoryBaseChunkType*>());
				{
					const std::lock_guard<LockType> lockBucket(fsmutex);//lock free store
					for (auto& e : fs->getContext())
					{
						for (auto ee = e.second.begin(); ee != e.second.end(); )
						{
							if ((*ee)->empty())
							{
								emptyBlocksSize -= (*ee)->getMemorySize();
								emptyBlocksCount -= 1;
								auto tmpee = ee;
								++tmpee;
								toDeleteList.splice(toDeleteList.begin(), e.second, ee);
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
					auto& bmutex = bucket->lock;
					{
						const std::lock_guard<LockType> lockBucket(bmutex);//lock bucket
						auto it = bucket->chunks.find(mrange);
						if (it == bucket->chunks.end())
							assert(false);
						else
						{
							bucket->chunks.erase(it);
						}
					}//unlock bucket
				}
			}
		}

		//if (force)
		//{
		//	memoryPool.clean(0, 0);
		//}
	
	}//unlock clean

	uint8_t* alloc(size_t size, size_t contextID = size_t(-1)) {
		
		auto SizeBucket = getSizeRange(size);//elementSize_perChunk_Map.find(MemoryRange{ size,size });
		//if (SizeBucketIt == elementSize_perChunk_Map.end())
		//	return nullptr;

		MemorySizeRange bucketminmax = SizeBucket.range;
		size_t bucketElemCount = SizeBucket.elPerChunk;
		auto chunkType = SizeBucket.chunkType;
		size_t buferSize = SizeBucket.buforSize;

		auto context = getAllocContext(contextID);
		auto contexttmp = context;

		auto fsContext = getFreeStoreContext(contextID);
		auto& fs = freeStore[fsContext];

		auto* cmutexptr = &notfullchunks[context]->getLock();
		cmutexptr->lock();
		{
			auto& cmutex = notfullchunks[context]->getLock();
			auto& notfull = notfullchunks[context]->getContext();

			const std::lock_guard<LockType> lockContext(cmutex, std::adopt_lock_t());//lock context

			auto notusedIt = notfull.find(bucketminmax);
			if (notusedIt != notfull.end() && !notusedIt->second.empty())
			{
				auto* chunk = notusedIt->second.front();

				auto bucketID = getMemoryBucket(reinterpret_cast<uint8_t*>(chunk->getAddresAsInt()));
				auto& bmutex = memory[bucketID]->lock;

				{
					const std::lock_guard<LockType> lockBucket(bmutex);//lock bucket
					bool wasEmpty = chunk->empty();
					
					auto ptr = chunk->alloc(size);
					if (ptr == nullptr)
					{
						assert(false);
						throw std::bad_alloc();
					}
					if (wasEmpty)
					{
						emptyBlocksSize -= chunk->getMemorySize();
						emptyBlocksCount -= 1;
					}
					if (chunk->full())
					{
						auto& srclist = notusedIt->second;
						auto& dstlist = chunk->freelistNode;
						dstlist.splice(dstlist.begin(), srclist, srclist.begin());
					}
					return ptr;
				}//unlock bucket
			}
			else
			{
				MemoryUniquePointerType uptr = nullptr;
				ChunksRawPointerType rptr = nullptr;
				
				typename MemoryContext<LockType>::ValueType templist(allocators.getInternalAllocatorLightTyped<MemoryChunkBase*>());


				auto& fsmutex = fs->getLock(); 
				fsmutex.lock();;
				{
					const std::lock_guard<LockType> lockFreeStore(fsmutex, std::adopt_lock);//lock free store
					auto& fstore = fs->getContext();
					auto fsit = fstore.find(bucketminmax);
					if (fsit != fstore.end() && !fsit->second.empty())
					{
						auto &fslist  = fsit->second;
						auto bestit = fslist.begin();
						auto bestptr = reinterpret_cast<void*>(size_t(-1));
						
						//find chunk with good size (anySize chunk case!), 
						//and smallest address (it possibly lower fragmentation) 

						size_t matchingCounter = 0;
						for (auto it = fslist.begin(); it != fslist.end(); ++it)
						{
							auto chsize = (*it)->getMemorySize();
							if ((chunkType == MemoryChunkType::AnySize ?
								((chsize >= size) && (chsize * 0.95 <= size)) :
								true))
							{
								bestptr = *it;
								bestit = it;
								break;
							}
						}
						if (bestit != fslist.end())
						{
							rptr = *bestit;
							templist.splice(templist.begin(), fslist, bestit);
						}
					}
				}//unlock free store

				if (!rptr)
				{
					uptr.reset(createNewChunk(SizeBucket, size).release());
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
					auto& bmutex = memory[bucketID]->lock;

					uint8_t* ptr = nullptr;
					ChunksRawPointerType chunkaddress = nullptr;
					
					if(uptr)
						chunkaddress = uptr.get();
					else
						chunkaddress = rptr;

					{
						const std::lock_guard<LockType> lockBucket(bmutex);//lock bucket
						bool wasEmpty;
						if (uptr)
						{
							wasEmpty = uptr->empty();
							ptr = uptr->alloc(size);
						}
						else
						{
							wasEmpty = rptr->empty();
							ptr = rptr->alloc(size);
						}

						if (ptr == nullptr)
						{
							assert(false);
							//it should never happen, but...
							std::printf("\tERROR: MemoryManagerParallel::alloc(), bug, that should never exist\n");
							throw std::bad_alloc();
						}

						if (uptr)
						{
							if (!uptr->full())
							{
								auto& dstlist = notfullchunks[context]->getChunkList(bucketminmax);
								auto& srclist = chunkaddress->freelistNode;
								dstlist.splice(dstlist.begin(), srclist, srclist.begin());
							}
							else
							{
								//SPLICE
							}
							assert(memory[bucketID]->chunks.count(memrange) == 0);
							memory[bucketID]->chunks[memrange].reset(uptr.release());
						}
						else
						{
							if (!rptr->full())
							{
								auto& dstlist = notfullchunks[context]->getChunkList(bucketminmax);
								auto& srclist = templist;
								dstlist.splice(dstlist.begin(), srclist, srclist.begin());
							}
							else
							{
								auto& dstlist = rptr->freelistNode;
								auto& srclist = templist;
								dstlist.splice(dstlist.begin(), srclist, srclist.begin());
							}
							if (wasEmpty)
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

		auto chunkIt2 = MemoryMapType::iterator();
		MemorySizeRangeInfo bucketRR2;
		bool needClean = false;
		bool isChunktToFreestore = false;

		auto context = getAllocContext(contextID);
		auto bucketID = getMemoryBucket(ptr);

		auto fsContext = getFreeStoreContext(contextID);
		auto& fs = freeStore[fsContext];

		size_t iteration = 0;
		while (iteration < BucketCount)
		{
			auto& bmutex = memory[bucketID]->lock;
			auto& bucket = memory[bucketID]->chunks;
			{
				const std::lock_guard<LockType> lockBucket(bmutex);//lock bucket

				auto chunkIt = bucket.find(
					MemoryRange{
						reinterpret_cast<size_t>(ptr),
						reinterpret_cast<size_t>(ptr) });
				if (chunkIt == bucket.end())
				{
					iteration++;
					bucketID = ((bucketID == 0) ? BucketCount : bucketID) - 1;
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
					needClean = true;
				}
				else if (dealocFromChunkResult == DealocResult::WAS_FULL_CHUNK)
				{
					auto elemsize = chunkIt->second->elSize();
					auto bucketRR = getSizeRange(elemsize);

					chunkIt2 = chunkIt;
					bucketRR2 = bucketRR;
					isChunktToFreestore = true;
				}
				else if (dealocFromChunkResult == DealocResult::WAS_FULL_IS_EMPTY)
				{
					auto elemsize = chunkIt->second->elSize();
					auto bucketRR = getSizeRange(elemsize);

					chunkIt2 = chunkIt;
					bucketRR2 = bucketRR;
					needClean = true;
					isChunktToFreestore = true;
					emptyBlocksSize += chunkIt->second->getMemorySize();
					emptyBlocksCount += 1;
				}
				else
				{
					assert(false);
					return;
				}
			}//unlock bucket

			if (isChunktToFreestore) {
				auto& fsmutex = fs->getLock();
				{
					const std::lock_guard<LockType> lockFreeStore(fsmutex);//lock free store

					auto& dstlist = fs->getChunkList(bucketRR2.range);
					auto& srclist = chunkIt2->second.get()->freelistNode;
					dstlist.splice(dstlist.begin(), srclist, srclist.begin());
				}//unlock free store
			}
			if (needClean)
			{
				clean(); 
			}
			return;
		} 
		
		assert(false);
		return;
	}

	MemoryAllocationInfo getInfo(uint8_t* ptr, size_t contextID = size_t(-1))
	{
		if (ptr == nullptr)
			return MemoryAllocationInfo();

		auto chunkIt2 = MemoryMapType::iterator();
		MemorySizeRangeInfo bucketRR2;
		bool needClean = false;
		bool isChunktToFreestore = false;

		//auto context = getAllocContext(contextID);
		auto bucketID = getMemoryBucket(ptr);

		//auto fsContext = getFreeStoreContext(contextID);
		//auto& fs = freeStore[fsContext];

		size_t iteration = 0;
		while (iteration < BucketCount)
		{
			auto& bmutex = memory[bucketID]->lock;
			auto& bucket = memory[bucketID]->chunks;
			{
				const std::lock_guard<LockType> lockBucket(bmutex);//lock bucket

				auto chunkIt = bucket.find(
					MemoryRange{
						reinterpret_cast<size_t>(ptr),
						reinterpret_cast<size_t>(ptr) });
				if (chunkIt == bucket.end())
				{
					iteration++;
					bucketID = ((bucketID == 0) ? BucketCount : bucketID) - 1;
					continue;
				}

				auto result = chunkIt->second->getInfo(ptr);

				return MemoryAllocationInfo(result);
			}//unlock bucket

			return MemoryAllocationInfo();
		}
		return MemoryAllocationInfo();;
	}

};

template <typename T1, typename LockType>
class MemoryManagerAllocator
{

	template <typename T1, typename T2, typename LockType>
	friend bool operator==(const MemoryManagerAllocator<T1, LockType>& A, const MemoryManagerAllocator<T2, LockType>& B) noexcept;
	template <typename T1, typename T2, typename LockType>
	friend bool operator!=(const MemoryManagerAllocator<T1, LockType>& A, const MemoryManagerAllocator<T2, LockType>& B) noexcept;

	template <typename T2, typename LockType>
	friend class MemoryManagerAllocator;

	MemoryManagerParallel<LockType>& manager;

public:

	typedef T1 value_type;

	MemoryManagerAllocator(MemoryManagerParallel<LockType>& manager) noexcept :manager(manager) {}

	template <typename T2>
	MemoryManagerAllocator(MemoryManagerAllocator<T2, LockType>& other) noexcept :
		manager(other.manager) {
	}

	template <typename T2>
	MemoryManagerAllocator(MemoryManagerAllocator<T2, LockType>&& other) noexcept :
		manager(other.manager) {
	}

	template <typename T2>
	MemoryManagerAllocator(const MemoryManagerAllocator<T2, LockType>& other) noexcept :
		manager(other.manager) {
	}

	template <typename T2>
	MemoryManagerAllocator(const MemoryManagerAllocator<T2, LockType>&& other) noexcept :
		manager(other.manager) {
	}

	T1* allocate(size_t n)
	{
		if (n > (std::numeric_limits<std::size_t>::max)() / sizeof(T1))
			throw std::bad_alloc();

		if (auto p = static_cast<T1*>((void*)manager.alloc(n * sizeof(T1))))
			return p;

		throw std::bad_alloc();
	}

	void deallocate(T1* ptr, size_t n) noexcept
	{
		manager.dealloc((uint8_t*)ptr);
	}
};

template <typename T1, typename T2, typename LockType>
inline bool operator==(const MemoryManagerAllocator<T1, LockType>& A, const MemoryManagerAllocator<T2, LockType>& B) noexcept {
	return &A.manager == &B.manager;
}

template <typename T1, typename T2, typename LockType>
inline bool operator!=(const MemoryManagerAllocator<T1, LockType>& A, const MemoryManagerAllocator<T2, LockType>& B) noexcept {
	return &A.manager != &B.manager;
}




















