#pragma once
#include "MemoryChunkPool.h"
#include "MemoryRange.h"
#include <map>
#include <set>
#include <vector>
#include <mutex>

class MemoryChunkPoolAllocator
{
	//static constexpr size_t sizeroundTable[] = {
	//	size_t(-1),
	//	0,1,2,3,			// 1  2  3  4
	//	4,5,6,7,			// 5  6  7  8
	//	8,9,10,10,			// 9 10 11 12
	//	14,14,15,16,		//13 14 15 16
	//	18,18,20,20,		//17 18 19 20
	//	21,24,24,24,		//21 22 23 24
	//	25,28,28,28,		//25 26 27 28
	//	30,30,32,32			//29 30 31 32
	//};
	static constexpr size_t sizetranslationTable[] = {
		size_t(-1),
		0,1,2,3,			// 1  2  3  4
		4,5,6,7,			// 5  6  7  8
		8,9,10,10,			// 9 10 11 12
		11,11,12,13,		//13 14 15 16
		14,14,15,15,		//17 18 19 20
		16,17,17,17,		//21 22 23 24
		18,19,19,19,		//25 26 27 28
		20,20,21,21			//29 30 31 32
	};
	static constexpr size_t sizeToPoolType[] = {
		0,
		32,32,72,32,		// 1  2  3  4
		30,72,84,32,		// 5  6  7  8
		72,30,72,72,		// 9 10 11 12
		84,84,30,32,		//13 14 15 16
		72,72,100,100,		//17 18 19 20
		84,72,72,72,		//21 22 23 24
		100,84,84,84,		//25 26 27 28
		30,30,32,32			//29 30 31 32
	};
	std::map<MemoryRange, std::unique_ptr<MemoryPool>> allPools;
	std::array<std::set<MemoryPool*>, 32> withoutDividing;
	std::array<std::set<MemoryPool*>, 32> withDividing;
	std::vector<std::unique_ptr<MemoryPool>> emptyPools;

	std::mutex mutex;

	size_t allocatedSize = 0;
	size_t allocatedCount = 0;
	size_t allocatedMemory = 0;

	void repairArrays(MemoryPool* ptr,
		std::pair<uint32_t, uint32_t> oldFreeTypes,
		std::pair<uint32_t, uint32_t> newFreeTypes)
	{
		if (oldFreeTypes == newFreeTypes)
			return;
		
		for (int i = 0; i < 32; i++)
		{
			if ((oldFreeTypes.first & (1 << i)) != (newFreeTypes.first & (1 << i)))
			{
				
				if ((oldFreeTypes.first & (1 << i)) == 0)//FIRST 0 -> 1
				{
					auto res = withoutDividing[i].insert(ptr);
					assert(res.second);
				}
				else//FIRST 1 -> 0
				{
					auto res = withoutDividing[i].erase(ptr);
					assert(res);
				}
			}

			if ((oldFreeTypes.second & (1 << i)) != (newFreeTypes.second & (1 << i)))
			{

				if ((oldFreeTypes.second & (1 << i)) == 0)//SECOND 0 -> 1
				{
					auto res = withDividing[i].insert(ptr);
					assert(res.second);
				}
				else//SECOND 1 -> 0
				{
					auto res = withDividing[i].erase(ptr);
					assert(res);
				}
			}
		}
	}

	static constexpr size_t roundSize(size_t size) noexcept 
	{
		return MemoryPool::getRoundSizeTable()[size];
	}
	static constexpr size_t translateRoundedSizeToIndex(size_t size) noexcept
	{
		return sizetranslationTable[roundSize(size)];
	}

public:
	uint8_t* alloc(size_t size, size_t recursionCounter = 0) noexcept {
		std::unique_lock<std::mutex> mutexlock(mutex,std::defer_lock_t());
		if (recursionCounter == 0)
			mutexlock.lock();

		assert((size & 0x7FF) == 0);

		const size_t blockNumberPrecise = size / 2048;

		assert((blockNumberPrecise >= 1) && (blockNumberPrecise <= 32));
		const size_t blockNumberRounded = roundSize(blockNumberPrecise);
		const size_t blockNumberTranslated = translateRoundedSizeToIndex(blockNumberPrecise);
		
		uint8_t* ptr = nullptr;

		if (!withoutDividing[blockNumberRounded-1].empty())
		{
			auto e = *withoutDividing[blockNumberRounded - 1].begin();
			assert(e);
			auto oldFreeTypes = e->getFreeSizes();
			size_t allocatedSizeBefore = e->getAllocatedSize(); 
			size_t allocatedCountBefore = e->getAllocatedCount();
			ptr = e->alloc(blockNumberRounded);
			size_t allocatedSizeAfter = e->getAllocatedSize();
			size_t allocatedCountAfter = e->getAllocatedCount();
			auto newFreeTypes = e->getFreeSizes();
			assert(ptr);
			repairArrays(e,oldFreeTypes,newFreeTypes);
			allocatedSize += allocatedSizeAfter - allocatedSizeBefore;
			allocatedCount += allocatedCountAfter - allocatedCountBefore;
		}
		else if (!withDividing[blockNumberRounded - 1].empty())
		{
			auto e = *withDividing[blockNumberRounded - 1].begin();
			assert(e);
			auto oldFreeTypes = e->getFreeSizes();
			size_t allocatedSizeBefore = e->getAllocatedSize();
			size_t allocatedCountBefore = e->getAllocatedCount();
			ptr = e->alloc(blockNumberRounded);
			size_t allocatedSizeAfter = e->getAllocatedSize();
			size_t allocatedCountAfter = e->getAllocatedCount();
			auto newFreeTypes = e->getFreeSizes();
			assert(ptr);
			repairArrays(e, oldFreeTypes, newFreeTypes);
			allocatedSize += allocatedSizeAfter - allocatedSizeBefore;
			allocatedCount += allocatedCountAfter - allocatedCountBefore;
		}
		else
		{
			assert(recursionCounter == 0);
			//std::cout << "AllocPool " << blockNumberRounded << "\n";
			std::unique_ptr<MemoryPool> newpool;
			if (emptyPools.empty())
			{
				try {
					newpool = std::make_unique<MemoryPool>();
				}
				catch (std::bad_alloc& e)
				{
					return nullptr;
				}
			}
			else {
				newpool = std::move(emptyPools.back());
				emptyPools.pop_back();
			}
			auto pooltype = sizeToPoolType[blockNumberRounded];
			if(!newpool->reset(pooltype))
				return nullptr;
			auto* ptr2 = newpool.get();
			assert(allPools.count(ptr2->getRange()) == 0);
			allPools[newpool->getRange()] = std::move(newpool);
			assert(allPools.count(ptr2->getRange()) == 1);
			repairArrays(ptr2, {0,0}, ptr2->getFreeSizes());
			
			ptr = alloc(size,recursionCounter+1);
			allocatedMemory += bool(ptr);
		}

		return ptr;
	}
	void dealloc(uint8_t* ptr) noexcept {
		const std::lock_guard<std::mutex> mutexlock(mutex);

		if (ptr == nullptr) 
			return;
		auto it = allPools.find( MemoryRange(ptr,ptr));
		if (it == allPools.end())
		{
			assert(false);
			return;
		}
		auto oldFreeTypes = it->second->getFreeSizes();

		size_t allocatedSizeBefore = it->second->getAllocatedSize();
		size_t allocatedCountBefore = it->second->getAllocatedCount();
		it->second->dealloc(ptr);
		
		size_t allocatedSizeAfter = it->second->getAllocatedSize();
		size_t allocatedCountAfter = it->second->getAllocatedCount();

		auto newFreeTypes = it->second->getFreeSizes();
		

		if (0 == allocatedCountAfter)
		{
			repairArrays(it->second.get(), oldFreeTypes, {0,0});
			auto ptr = std::move(it->second);
			allPools.erase(it);
			emptyPools.push_back(std::move(ptr));
			clean();
		}
		else
		{
			repairArrays(it->second.get(), oldFreeTypes, newFreeTypes);
		}

		allocatedSize += allocatedSizeAfter - allocatedSizeBefore;
		allocatedCount += allocatedCountAfter - allocatedCountBefore;
	}

	void clean(size_t shouldBe = 8, size_t tolerance = 8) noexcept
	{
		const std::lock_guard<std::mutex> mutexlock(mutex);

		if (emptyPools.size() > shouldBe + tolerance)
		{
			allocatedMemory -= emptyPools.size()- shouldBe;
			emptyPools.erase(emptyPools.begin() + shouldBe, emptyPools.end());

		}
	}

	bool correctnessTest() const noexcept {
		for(const auto &e: allPools)
			return e.second->correctnessTest();
	}
};

