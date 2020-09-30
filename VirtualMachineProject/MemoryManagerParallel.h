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




constexpr size_t MEMORY_BUCKET_SIZE_BITS = 15;
constexpr size_t MEMORY_BUCKET_COUNT_BITS = 7;
constexpr size_t MEMORY_ALLOC_CONTEXT_COUNT_BITS = 7;
constexpr size_t MEMORY_FREESTORE_COUNT = 8;



constexpr size_t MEMORY_BUCKET_SIZE =
		size_t(1) << MEMORY_BUCKET_SIZE_BITS;

constexpr size_t MEMORY_BUCKET_COUNT = 
		size_t(1) << MEMORY_BUCKET_COUNT_BITS;

constexpr size_t MEMORY_ALLOC_CONTEXT_COUNT = 
		size_t(1) << MEMORY_ALLOC_CONTEXT_COUNT_BITS;









template<class PoolAllocatorType, class InternalStructAllocatorType, class LockType>
class MemoryManagerParallel
{
	//std::atomic_size_t counterssssssssss[10] = {};
	using MemoryManagerType = MemoryManagerParallel<PoolAllocatorType, InternalStructAllocatorType, LockType>;

	template <typename T>
	struct ChunkDeleterT {
		//AT& allocator;
		
		//ChunkDeleterT(InternalStructAllocatorType& allocator) :allocator(allocator) {}
		ChunkDeleterT() = default;
		ChunkDeleterT(const ChunkDeleterT& other) = default;
		//ChunkDeleterT(ChunkDeleterT& other) = default;
		ChunkDeleterT(ChunkDeleterT&& other) = default;
		~ChunkDeleterT() = default;
	
		void operator()(T* ptr) const noexcept {
			if (ptr)
			{
				auto &mm = ptr->getMemoryManager();
				ptr->~T();
				mm.deallocInternalStructMemory(reinterpret_cast<uint8_t*>(ptr));
			}
			
		}
	};

	template <typename T, typename AT>
	struct CustomAllocator
	{
		typedef T value_type;
		
		constexpr CustomAllocator() = default;
		
		template <typename U> 
		CustomAllocator(const CustomAllocator <U, AT>& other) noexcept : 
			allocator(other.allocator) {}
		[[nodiscard]] T* allocate(std::size_t n) {
			if (n > (std::numeric_limits<std::size_t>::max)() / sizeof(T))
				throw std::bad_alloc();

			if (auto p = reinterpret_cast<T*>(allocator.alloc(n * sizeof(T)))) {
				return p;
			}
			throw std::bad_alloc();
		}

		void deallocate(T* p, std::size_t n) noexcept {
			allocator.dealloc(p);
		}
		AT& allocator;
	};

	using MemoryAddressRange = MemoryRange; 
	using MemorySizeRange = MemoryRange;
	//using MemoryBaseChunkType = MemoryChunkBase<PoolAllocatorType>;
	using MemoryBaseChunkType = MemoryChunkBase<MemoryManagerType>;
	using DeleterInternalType = ChunkDeleterT<MemoryBaseChunkType>;
	///using DeleterPoolType = ChunkDeleterT<MemoryBaseChunkType >;

	using MemoryUniquePointerType = std::unique_ptr<MemoryBaseChunkType, DeleterInternalType>;
	using MemoryMapType = std::map< MemoryAddressRange, MemoryUniquePointerType >;
	using MemoryBucketType = std::pair< LockType, MemoryMapType >;
	using MemoryBucketArrayType = std::array<MemoryBucketType, MEMORY_BUCKET_COUNT>;
	using ChunksRawPointerType = MemoryBaseChunkType*;
	using ChunksListType = std::list<ChunksRawPointerType>;
	using ChunksMapType = std::map<MemorySizeRange, ChunksListType>;
	using ContextType = std::pair<LockType, ChunksMapType>;
	using ContextArrayType = std::array<ContextType, MEMORY_ALLOC_CONTEXT_COUNT>;

	using ContextFreeStoreArrayType = std::array<ContextType, MEMORY_FREESTORE_COUNT>;


	std::pair<LockType, ChunksListType> listNodeStore;
	ChunksListType getListNode() {
		std::lock_guard<LockType> lock(listNodeStore.first);
		ChunksListType result;
		if (!listNodeStore.second.empty())
		{
			result.splice(result.begin(), listNodeStore.second, listNodeStore.second.begin());
		}
		else
		{
			result.resize(1);
		}
		return result;
	}

	void storeListNode(ChunksListType&& other)
	{
		std::lock_guard<LockType> lock(listNodeStore.first);
		listNodeStore.second.splice(listNodeStore.second.begin(), other);
	}

	//contains all memory chunks, indexed by address range
	MemoryBucketArrayType memory;
	//std::array<std::pair< LockType, std::map< MemoryAddressRange, MemoryChunkBase<PoolAllocatorType>*> >, MEMORY_BUCKET_COUNT> memory;

	//contains not full chunks, binded to context, indexed by chunksize
	ContextArrayType notfullchunks;
	//std::array<std::pair<LockType, std::map<MemorySizeRange, std::list<MemoryChunkBase<PoolAllocatorType>*>>>, MEMORY_ALLOC_CONTEXT_COUNT> notfullchunks;

	//contains not full chunks, not binded to any context, indexed by chunksize
	ContextFreeStoreArrayType freeStore;

	/// std::pair<LockType, std::map<MemorySizeRange, std::list<MemoryChunkBase<PoolAllocatorType>*>>> freeStore;
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
	MemoryUniquePointerType createNewChunk(MemorySizeRangeInfo msri, size_t size)
	{
		//MemoryUniquePointerType uptr = { }; //, DeleterType(internalAlloc)
		
		if (msri.chunkType == MemoryChunkType::Precise)
		{
			using ChunkType = MemoryChunkPrecise<MemoryManagerType>;
			auto* buf = internalAlloc.alloc(sizeof(ChunkType));
			ChunkType* rawptr = new (buf) ChunkType(
				*this, msri.buforSize, msri.range.maxv, msri.elPerChunk);
			assert((void*)buf == (void*)rawptr);
			return MemoryUniquePointerType(static_cast<MemoryBaseChunkType*>(rawptr));
			//uptr.reset(static_cast<MemoryBaseChunkType*>(rawptr));
		}
		else if (msri.chunkType == MemoryChunkType::VerySmall)
		{
			using ChunkType = MemoryChunkVerySmall<MemoryManagerType>;
			auto* buf = internalAlloc.alloc(sizeof(ChunkType));
			ChunkType* rawptr = new (buf) ChunkType(
				*this, msri.buforSize, msri.range.maxv, msri.elPerChunk);
			assert((void*)buf == (void*)rawptr);
			return MemoryUniquePointerType(static_cast<MemoryBaseChunkType*>(rawptr));
			//uptr.reset(static_cast<MemoryBaseChunkType*>(rawptr));
		}
		else if (msri.chunkType == MemoryChunkType::Small)
		{
			using ChunkType = MemoryChunkSmall<MemoryManagerType>;
			auto* buf = internalAlloc.alloc(sizeof(ChunkType));
			ChunkType* rawptr = new (buf) ChunkType(
				*this, msri.buforSize, msri.range.maxv, msri.elPerChunk);
			assert((void*)buf == (void*)rawptr);
			return MemoryUniquePointerType(static_cast<MemoryBaseChunkType*>(rawptr));
			//uptr.reset(static_cast<MemoryBaseChunkType*>(rawptr));
		}
		else if (msri.chunkType == MemoryChunkType::Medium)
		{
			using ChunkType = MemoryChunkMedium<MemoryManagerType>;
			auto* buf = internalAlloc.alloc(sizeof(ChunkType));
			ChunkType* rawptr = new (buf) ChunkType(
				*this, msri.buforSize, msri.range.maxv, msri.elPerChunk);
			assert((void*)buf == (void*)rawptr);
			return MemoryUniquePointerType(static_cast<MemoryBaseChunkType*>(rawptr));
			//uptr.reset(static_cast<MemoryBaseChunkType*>(rawptr));
		}
		else if (msri.chunkType == MemoryChunkType::Large)
		{
			using ChunkType = MemoryChunkLarge<MemoryManagerType>;
			auto* buf = internalAlloc.alloc(sizeof(ChunkType));
			ChunkType* rawptr = new (buf) ChunkType(
				*this, msri.buforSize, msri.range.maxv, msri.elPerChunk);
			assert((void*)buf == (void*)rawptr);
			return MemoryUniquePointerType(static_cast<MemoryBaseChunkType*>(rawptr));
			//uptr.reset(static_cast<MemoryBaseChunkType*>(rawptr));
		}
		else if (msri.chunkType == MemoryChunkType::AnySize)
		{
			using ChunkType = MemoryChunkAnySize<MemoryManagerType>;
			auto* buf = internalAlloc.alloc(sizeof(ChunkType));
			ChunkType* rawptr = new (buf) ChunkType(
				*this, size);
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

	static size_t getFreeStoreContext(size_t contextID = size_t(-1)) noexcept
	{
		if (contextID == size_t(-1))
		{
			auto thid = std::this_thread::get_id();
			std::hash<std::thread::id> hasher;
			contextID = hasher(thid);
		}
		return (contextID + 1999) % MEMORY_FREESTORE_COUNT;
	}

public:
	uint8_t* allocPoolMemory(size_t size) {
		return reinterpret_cast<uint8_t*>(memoryPool.alloc(size));
	}
	void deallocPoolMemory(uint8_t* ptr) {
		memoryPool.dealloc(ptr);
	}
	uint8_t* allocInternalStructMemory(size_t size) {
		return reinterpret_cast<uint8_t*>(internalAlloc.alloc(size));
	}
	void deallocInternalStructMemory(uint8_t* ptr) {
		internalAlloc.dealloc(ptr);
	}


	MemoryManagerParallel() = default;
	MemoryManagerParallel(const PoolAllocatorType &pool): memoryPool(pool) { }
	MemoryManagerParallel(PoolAllocatorType&& pool) : memoryPool(std::move(pool)) { }

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

			auto& fsmutex = fs.first;

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
								//emptyBlocksSize -= (*chunkit)->getMemorySize();
								//emptyBlocksCount -= 1;
								auto ptr = (*chunkit);
								MemoryAddressRange mrange = { ptr->getAddresAsInt(), ptr->getAddresAsInt()+ ptr->getMemorySize()-1 };
								fs.second[sizeRange.first].push_back(ptr);
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
				std::list<MemoryChunkBase<MemoryManagerType>* > toDeleteList;
				{
					const std::lock_guard<LockType> lockBucket(fsmutex);//lock free store
					for (auto& e : fs.second)
					{
						for (auto ee = e.second.begin(); ee != e.second.end(); )
						{
							if ((*ee)->empty())
							{
								//counterssssssssss[0]++;
								
								//auto oldvalue1 = 
								emptyBlocksSize -= (*ee)->getMemorySize();
								//auto oldvalue2 = 
								emptyBlocksCount -= 1;
								//if (oldvalue1 == 0 || oldvalue2 == 0)
								//	std::cout << oldvalue2<<"\n";
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
						{
							//auto ptrtodelete = std::move(it->second);
							//ptrtodelete->~MemoryBaseChunkType();
							//deallocInternalStructMemory internalAlloc.dealloc(reinterpret_cast<uint8_t*>( ptrtodelete.get()));
							bucket.second.erase(it);
						}
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

		MemorySizeRange bucketminmax = SizeBucket.range;
		size_t bucketElemCount = SizeBucket.elPerChunk;
		auto chunkType = SizeBucket.chunkType;
		size_t buferSize = SizeBucket.buforSize;

		auto context = getAllocContext(contextID);
		auto contexttmp = context;

		auto fsContext = getFreeStoreContext(contextID);
		auto& fs = freeStore[fsContext];

		//size_t counter = 3;
		//while (counter) {
		//	auto& cmutexptr = notfullchunks[contexttmp].first;
		//
		//	if (cmutexptr.try_lock())
		//	{
		//		context = contexttmp;
		//		break;
		//	}
		//	contexttmp = getNextAllocContext(contexttmp);
		//	--counter;
		//}
		//
		//if (counter == 0)
		//{
		//	auto* cmutexptr = &notfullchunks[context].first;
		//	cmutexptr->lock();
		//}
		auto* cmutexptr = &notfullchunks[context].first;
		cmutexptr->lock();
		{
			auto& cmutex = notfullchunks[context].first;
			auto& notfull = notfullchunks[context].second;

			const std::lock_guard<LockType> lockContext(cmutex, std::adopt_lock_t());//lock context

			auto notusedIt = notfull.find(bucketminmax);
			if (notusedIt != notfull.end() && !notusedIt->second.empty())
			{
				auto* chunk = notusedIt->second.front();

				auto bucketID = getMemoryBucket(reinterpret_cast<uint8_t*>(chunk->getAddresAsInt()));
				auto& bmutex = memory[bucketID].first;

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
						//counterssssssssss[1]++;
						//auto oldvalue1 = 
						emptyBlocksSize -= chunk->getMemorySize();
						//auto oldvalue2 = 
						emptyBlocksCount -= 1;
						//if (oldvalue1 == 0 || oldvalue2 == 0)
						//	std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!2\n";
					}
					if (chunk->full())
						notusedIt->second.pop_front();
					return ptr;
				}//unlock bucket
			}
			else
			{
				//std::unique_ptr<MemoryChunkBase<PoolAllocatorType>> uptr;
				MemoryUniquePointerType uptr = nullptr;
				ChunksRawPointerType rptr = nullptr;

				auto& fsmutex = fs.first; fsmutex.lock();;
				//if ()//if (fsmutex.try_lock())
				{
					const std::lock_guard<LockType> lockFreeStore(fsmutex, std::adopt_lock);//lock free store
					auto& fstore = fs.second;
					auto fsit = fstore.find(bucketminmax);
					if (fsit != fstore.end() && !fsit->second.empty())
					{
						auto &fslist  = fsit->second;
						auto bestit = fslist.begin();
						auto bestptr = reinterpret_cast<void*>(size_t(-1));
						
						//find chunk with good size (anySize chunk case!), 
						//and smallest address (it possibly lower fragmentation) 

						size_t matchingCounter = 0;
						//static size_t maxMatchingCounter = 0;
						for (auto it = fslist.begin(); it != fslist.end(); ++it)
						{
							auto chsize = (*it)->getMemorySize();
							if ( (*it < bestptr) && (chunkType == MemoryChunkType::AnySize ?
								((chsize >= size) && (chsize * 0.95 <= size)) :
								true))
							{
								bestptr = *it;
								bestit = it;
								//++matchingCounter;
								//if (maxMatchingCounter < matchingCounter)
								//	maxMatchingCounter = matchingCounter;
								//if (matchingCounter == 3)
								//	break;
							}
						}
						if (bestit != fslist.end())
						{
							rptr = *bestit;
							fslist.erase(bestit);
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
					auto& bmutex = memory[bucketID].first;

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
								notfull[bucketminmax].push_front(chunkaddress);

							assert(memory[bucketID].second.count(memrange) == 0);
							memory[bucketID].second[memrange].reset(uptr.release());
						}
						else
						{
							if (!rptr->full())
								notfull[bucketminmax].push_front(chunkaddress);

							if (wasEmpty)
							{
								//counterssssssssss[2]++;
								//auto oldvalue1 = 
								emptyBlocksSize -= rptr->getMemorySize();
								//auto oldvalue2 = 
								emptyBlocksCount -= 1;
								//if (oldvalue1 == 0 || oldvalue2 == 0)
								//	std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!3\n";
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

				
				//auto oldUseCount = chunkIt->second->getUseCount();//////DEBUG

				auto dealocFromChunkResult = chunkIt->second->dealloc(ptr);
				
				//auto newUseCount = chunkIt->second->getUseCount();//////DEBUG
				
				//if (dealocFromChunkResult == DealocResult::OK)//////DEBUG
				//{
				//	if(!(oldUseCount != 0 && newUseCount != 0))
				//		std::cout << "ERROR 1\n";//////DEBUG
				//}
				//if (dealocFromChunkResult == DealocResult::NOW_IS_EMPTY_CHUNK)//////DEBUG
				//{
				//	if (!(oldUseCount != 0 && newUseCount == 0))
				//		std::cout << "ERROR 2\n";//////DEBUG
				//}
				//if (dealocFromChunkResult == DealocResult::WAS_FULL_CHUNK)//////DEBUG
				//{
				//	if (!(oldUseCount != 0 && newUseCount != 0))
				//		std::cout << "ERROR 3\n";//////DEBUG
				//}
				//if (dealocFromChunkResult == DealocResult::WAS_FULL_IS_EMPTY)//////DEBUG
				//{
				//	if (!(oldUseCount != 0 && newUseCount == 0))
				//		std::cout << "ERROR 4\n";//////DEBUG
				//}


				if (dealocFromChunkResult == DealocResult::OK)
				{
					
					return;
				}
				else if (dealocFromChunkResult == DealocResult::NOW_IS_EMPTY_CHUNK)
				{
					//counterssssssssss[3]++;
					emptyBlocksSize += chunkIt->second->getMemorySize();
					emptyBlocksCount += 1;
					needClean = true;
				}
				else if (dealocFromChunkResult == DealocResult::WAS_FULL_CHUNK)
				{
					auto elemsize = chunkIt->second->elSize();
					auto bucketRR = getSizeRange(elemsize);

					//?
					chunkIt2 = chunkIt;
					bucketRR2 = bucketRR;
					isChunktToFreestore = true;
					//return;
				}
				else if (dealocFromChunkResult == DealocResult::WAS_FULL_IS_EMPTY)
				{
					auto elemsize = chunkIt->second->elSize();
					auto bucketRR = getSizeRange(elemsize);

					//?
					chunkIt2 = chunkIt;
					bucketRR2 = bucketRR;
					needClean = true;
					isChunktToFreestore = true;
					//counterssssssssss[4]++;
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
				auto& fsmutex = fs.first;
				{
					const std::lock_guard<LockType> lockFreeStore(fsmutex);//lock free store
					fs.second[bucketRR2.range].push_back(chunkIt2->second.get());
				}//unlock free store
			}
			if(needClean)
				clean();
			return;
		} 
		
		assert(false);
		return;
	}

};
