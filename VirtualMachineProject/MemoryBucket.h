#pragma once

#include <map>

#include <list>
#include <mutex>
#include <exception>


#include "MemoryRange.h"
#include "MemoryCustomAllocator.h"
#include "MemoryChunkBase.h"
#include "MemorySpinlock.h"

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
			auto &mm = ptr->getAllocators();
			ptr->~T();
			mm.deallocInternalStructMemory(reinterpret_cast<uint8_t*>(ptr), sizeof(T)*1);
		}
		
	}
};

template <typename T>
struct InternalStructDeleterT {
	//AT& allocator;

	//InternalStructDeleterT(InternalStructAllocatorType& allocator) :allocator(allocator) {}
	InternalStructDeleterT() = default;
	InternalStructDeleterT(const InternalStructDeleterT& other) = default;
	//InternalStructDeleterT(InternalStructDeleterT& other) = default;
	InternalStructDeleterT(InternalStructDeleterT&& other) = default;
	~InternalStructDeleterT() = default;

	void operator()(T* ptr) const noexcept {
		if (ptr)
		{
			auto& mm = ptr->getAllocators();
			ptr->~T();
			mm.deallocInternalStructMemory(reinterpret_cast<uint8_t*>(ptr));
		}

	}
};

template<class LockType>
class MemoryBucket
{
public:
	using MemoryChunkBaseType = MemoryChunkBase;
	using KeyType = MemoryRange;
	using ValueType = std::unique_ptr< MemoryChunkBaseType, ChunkDeleterT< MemoryChunkBaseType > >;
	using ComparatorType = std::less<KeyType>;
	using KeyValueType = std::pair<const KeyType, ValueType>;
	using AllocatorType = MemoryAllocatorLightTyped<KeyValueType>;

	MemoryCustomAllocator& allocator;
	LockType lock;
	std::map< KeyType, ValueType, ComparatorType, AllocatorType> chunks;
	std::list < MemoryChunkBaseType*, MemoryAllocatorLightTyped<MemoryChunkBaseType* > > fullChunkList;

	MemoryBucket(MemoryCustomAllocator& allocator):
		allocator(allocator),
		chunks(allocator.getInternalAllocatorLightTyped<KeyValueType>()),
		fullChunkList(allocator.getInternalAllocatorLightTyped<MemoryChunkBaseType*>())
	{}
	MemoryCustomAllocator& getAllocators() { return allocator; }
};

template<class LockType>
class MemoryContext
{
public:
	using MemoryChunkBaseType = MemoryChunkBase;
	using KeyType = MemoryRange;
	using ValueType = std::list <MemoryChunkBaseType*, MemoryAllocatorLightTyped<MemoryChunkBaseType*>>;
	using ComparatorType = std::less<KeyType>;
	using KeyValueType = std::pair<const KeyType, ValueType>;
	using AllocatorType = MemoryAllocatorLightTyped<KeyValueType>;
private:
	MemoryCustomAllocator& allocator;
	LockType lock;
	std::map< KeyType, ValueType, ComparatorType, AllocatorType> chunks;
public:
	MemoryContext(MemoryCustomAllocator& allocator) :
		allocator(allocator),
		chunks(allocator.getInternalAllocatorLightTyped<KeyValueType>())
	{}
	MemoryCustomAllocator& getAllocators() { return allocator; }
	LockType& getLock() { return lock; }
	std::map< KeyType, ValueType, ComparatorType, AllocatorType>& getContext() { 
		return chunks;
	}
	ValueType& getChunkList(const KeyType &key) 
	{
		auto it = chunks.find(key);
		if (it == chunks.end())
		{
			auto result = chunks.insert(std::pair<MemoryContext<LockType>::KeyType, MemoryContext<LockType>::ValueType>(
				key,
				MemoryContext<LockType>::ValueType(
					allocator.getInternalAllocatorLightTyped<KeyValueType>())));
			if (!result.second)
			{
				//std::cout << "ERROR TODO: failed create freestore list\n";
				//assert(false);
				throw std::bad_alloc();
			}
			return result.first->second;
		}
		return it->second; 
	}


};
