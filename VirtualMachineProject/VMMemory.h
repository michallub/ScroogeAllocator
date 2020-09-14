#pragma once
#include <vector>
#include <map>
#include <memory>
//#include "VMPointer.h"
#include <cassert>
#include <list>
#include <iostream>
class VMMemory;

class VMMemoryChunkBase {
	friend class VMMemory;
protected:
	std::unique_ptr<uint8_t[]> chunk;
	VMMemory* vmmemory = 0;
	size_t memorysize;
	size_t useCount = 0;

	//VMPointer createPointer(void* ptr) noexcept {
	//	assert(chunk.get() <= ptr);
	//	assert(chunk.get()+ memorysize > ptr);
	//	return VMPointer(ptr, this);
	//}
	//void* getAddress(VMPointer ptr) const noexcept { return ptr.ptr; }
public:
	enum class DealocResult{
		OK,
		NOW_IS_EMPTY_CHUNK,
		WAS_FULL_CHUNK,
		WAS_FULL_IS_EMPTY,
		BAD_ADDRESS,
		DEALOC_NOT_ALLOCATED
	};

	VMMemoryChunkBase(VMMemory* vmmemory, size_t memorysize) :
		vmmemory(vmmemory),
		chunk(std::make_unique< uint8_t[]>(memorysize)), 
		memorysize(memorysize){}
	bool empty() { return !useCount; }
	virtual bool full() const noexcept = 0;
	virtual uint8_t* alloc() = 0;
	virtual DealocResult dealloc(uint8_t* ptr) = 0;
	virtual ~VMMemoryChunkBase() = default;
	virtual size_t elSize() const noexcept = 0;
};



class VMMemoryChunkSmall: public VMMemoryChunkBase {
	uint64_t bitfield64[64] = {};
	uint64_t bitfield1 = {};
	uint64_t bitfieldempty = 0xFFFF'FFFF'FFFF'FFFF;
	uint64_t bitusedmask;
	size_t elementSize;
	size_t findFirstEmptyBit(uint64_t bits) const noexcept
	{
		static constexpr size_t helptable[16] = {
			0, // 0b0000
			1, // 0b0001
			0, // 0b0010
			2, // 0b0011
			0, // 0b0100
			1, // 0b0101
			0, // 0b0110
			3, // 0b0111
			0, // 0b1000
			1, // 0b1001
			0, // 0b1010
			2, // 0b1011
			0, // 0b1100
			1, // 0b1101
			0, // 0b1110
			-1 // 0b1111
		};

		size_t index = 0;
		if ((bits & 0xFFFF'FFFF) == (0xFFFF'FFFF))
		{
			index += 32;	bits >>= 32;
		}
		bits &= 0xFFFF'FFFF;

		if ((bits & 0xFFFF) == (0xFFFF))
		{
			index += 16;	bits >>= 16;
		}
		bits &= 0xFFFF;

		if ((bits & 0xFF) == (0xFF))
		{
			index += 8;	bits >>= 8;
		}
		bits &= 0xFF;

		if ((bits & 0xF) == (0xF))
		{
			index += 4;	bits >>= 4;
		}
		bits &= 0xF;

		index += helptable[bits];

		return index;
	}
public:

	VMMemoryChunkSmall(VMMemory* vmmemory, size_t memorysize, size_t elementSize, size_t elementCount):
		VMMemoryChunkBase(vmmemory, memorysize), elementSize(elementSize){ 
		switch (elementCount) {
		case 4096: bitusedmask = 0xFFFF'FFFF'FFFF'FFFF; break;
		case 2048: bitusedmask = 0x0000'0000'FFFF'FFFF; break;
		case 1024: bitusedmask = 0x0000'0000'0000'FFFF; break;
		case 512: bitusedmask  = 0x0000'0000'0000'00FF; break;
		case 256: bitusedmask  = 0x0000'0000'0000'000F; break;
		case 128: bitusedmask  = 0x0000'0000'0000'0003; break;
		case 64: bitusedmask   = 0x0000'0000'0000'0001; break;
		default: bitusedmask   = 0x0000'0000'0000'0000; elementCount = 0; break;
		}
		bitfieldempty &= bitusedmask;
		assert(memorysize == elementCount * elementSize);
	}
	bool full() const noexcept override { return bitfield1 == bitusedmask; }
	size_t elSize() const noexcept override {
		return elementSize;
	}
	uint8_t* alloc() override
	{
		if (full())
			return nullptr;

		size_t index1 = findFirstEmptyBit(bitfield1);
		size_t index2 = findFirstEmptyBit(bitfield64[index1]);
		size_t index = (index1 << 6) | index2;
		
		//std::cout << "\n###\n"<< bitfield64[index1] << "\n";
		//std::cout << (1 << index2) << "\n###\n";

		assert((bitfield64[index1] & (1ull << index2)) == 0);

		bitfieldempty &= ~(1 << index1);
		bitfield64[index1] |= (1ull << index2);
		if (bitfield64[index1] == 0xFFFF'FFFF'FFFF'FFFF)
		{
			assert((bitfield1 & (1ull << index1)) == 0);
			bitfield1 |= (1ull << index1);
		}
		useCount++;
		return chunk.get() + elementSize * index;//createPointer();
	}
	virtual DealocResult dealloc(uint8_t* ptr)
	{
		auto wasfull = full();
		auto addr = (uint8_t*)ptr - chunk.get();
		if (addr < 0 || addr >= memorysize) 
			return DealocResult::BAD_ADDRESS;
		size_t index = addr / elementSize;
		size_t index1 = index >> 6;
		size_t index2 = index & 0x3F;
		if (bitfield64[index1] == 0xFFFF'FFFF'FFFF'FFFF)
		{
			if ((bitfield1 & (1ull << index1)) == 0)
				return DealocResult::DEALOC_NOT_ALLOCATED;
			bitfield1 &= ~(1ull << index1);
		}
		if ((bitfield64[index1] & (1ull << index2)) == 0)
			return DealocResult::DEALOC_NOT_ALLOCATED;
		bitfield64[index1] &= ~(1ull << index2);
		if(bitfield64[index1] == 0)
			bitfieldempty |= (1ull << index1);
		useCount--;
		wasfull;
		bool isnowempty = ((bitfieldempty & bitusedmask) == bitusedmask);
		if(isnowempty)
			if(wasfull)
				return DealocResult::WAS_FULL_IS_EMPTY;
			else
				return DealocResult::NOW_IS_EMPTY_CHUNK;
		else
			if (wasfull)
				return DealocResult::WAS_FULL_CHUNK;
			else
				return DealocResult::OK;
		
	}
};



class VMMemoryChunkBig :public VMMemoryChunkBase {

public:
	VMMemoryChunkBig(VMMemory* vmmemory, size_t memorysize):
		VMMemoryChunkBase(vmmemory, memorysize)
	{

	}
	
	bool full() const noexcept override { return useCount; }
	uint8_t* alloc() override {
		if (empty()) { 
			++useCount;
			return chunk.get();// createPointer();
		}
		return nullptr;
	};
	DealocResult dealloc(uint8_t* ptr) override
	{
		if (chunk.get() == ptr)// getAddress(ptr))
		{
			if (useCount != 1)
				return DealocResult::DEALOC_NOT_ALLOCATED;
			--useCount;
			return DealocResult::WAS_FULL_IS_EMPTY;
		}
		return DealocResult::BAD_ADDRESS;
	}
	~VMMemoryChunkBig() = default;
	size_t elSize() const noexcept override {
		return memorysize;
	}

};


struct MemoryRange
{
	size_t minv, maxv;
	bool operator<(const MemoryRange& b) const noexcept
	{
		if (maxv < b.minv )
			return true;
		return false;
	}
};

class VMMemory
{
	
	constexpr static std::pair< MemoryRange, size_t> elementSize_perChunk_Table[]{
		{{1, 1}, 4096},
		{{2, 2}, 4096},
		{{3, 4}, 4096},
		{{5, 8}, 4096},
		{{9, 12}, 4096},
		{{13, 16}, 4096},
		{{17, 20}, 4096},
		{{21, 24}, 4096},
		{{25, 32}, 4096},
		{{33, 40}, 4096},
		{{41, 48}, 4096},
		{{49, 56}, 4096},
		{{57, 64}, 4096},
		{{65, 80}, 4096},
		{{81, 96}, 4096},
		{{97, 112}, 4096},
		{{113, 128}, 4096},
		{{129, 160}, 4096},
		{{161, 192}, 4096},
		{{193, 224}, 4096},
		{{225, 256}, 4096},
		{{257, 288}, 2048},
		{{289, 320}, 2048},
		{{321, 384}, 2048},
		{{385, 448}, 2048},
		{{449, 512}, 2048},
		{{513, 640}, 1024},
		{{641, 768}, 1024},
		{{769, 896}, 1024},
		{{897, 1024}, 1024},
		{{1025, 1280}, 512},
		{{1281, 1536}, 512},
		{{1537, 1792}, 512},
		{{1793, 2048}, 512},
		{{2049, 2560}, 256},
		{{2561, 3072}, 256},
		{{3073, 3584}, 256},
		{{3585, 4096}, 256},
		{{4097, 5120}, 128},
		{{5121, 6144}, 128},
		{{6145, 7168}, 128},
		{{7169, 8192}, 128},
		{{8193, 10240}, 64},
		{{10241, 12288}, 64},
		{{12289, 14336}, 64},
		{{14337, 16384}, 64},
		{{16385, size_t(-1)}, 1}
	};
	static std::map<MemoryRange, size_t > elementSize_perChunk_Map;
	
	static void staticinit() {
		if (elementSize_perChunk_Map.empty())
		{
			for (auto e : elementSize_perChunk_Table)
				elementSize_perChunk_Map.insert(e);
		}
	}

	std::map<MemoryRange, std::unique_ptr<VMMemoryChunkBase>> memory;
	std::map<MemoryRange, std::list<VMMemoryChunkBase*>> notfullchunks;
	size_t emptyBlocksSize = 0;
	size_t emptyBlocksCount = 0;

public:
	void clean( bool force = false) {
		if (((emptyBlocksSize >= 1024 * 1024 * 16) && (emptyBlocksCount >= 8) )
			|| (emptyBlocksSize >= 1024 * 1024 *256)
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
						emptyBlocksSize -= (*ee)->memorysize;
						emptyBlocksCount -= 1;
						MemoryRange mrange = { reinterpret_cast<size_t>((*ee)->chunk.get()), reinterpret_cast<size_t>((*ee)->chunk.get()) };
						auto it = memory.find(mrange);
						if (it == memory.end())
							assert(false);
						ee = e.second.erase(ee);

						memory.erase(it);

//						deletecounter++;
					}
					else
						ee++;
				}
			}
//			size_t copyafter = emptyBlocksSize;
//			std::cout << copybefore << " " << copyafter << " " << (copybefore - copyafter) << " " << deletecounter << "\n";
		}
	}

	VMMemory() { 
		staticinit();
	}

	uint8_t* alloc(size_t size) {
		auto bucketIt = elementSize_perChunk_Map.find(MemoryRange{size,size});
		if (bucketIt == elementSize_perChunk_Map.end())
			return nullptr;
		MemoryRange sizebucket = bucketIt->first;
		size_t bucketSize = bucketIt->second;
		auto notusedIt =  notfullchunks.find(sizebucket);
		if (notusedIt != notfullchunks.end() && !notusedIt->second.empty())
		{

			auto* chunk = notusedIt->second.front();
			if (chunk->empty())
			{
				emptyBlocksSize -= chunk->memorysize;
				emptyBlocksCount -= 1;
			}
			auto ptr = chunk->alloc();
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
			std::unique_ptr<VMMemoryChunkBase> uptr;
			if (bucketSize >= 64)
			{
				VMMemoryChunkSmall* rawptr = std::make_unique<VMMemoryChunkSmall>(
					this, bucketSize * sizebucket.maxv, sizebucket.maxv, bucketSize).release();
				uptr.reset(static_cast<VMMemoryChunkBase*>(rawptr));
			}
			else
			{
				VMMemoryChunkBig* rawptr = std::make_unique<VMMemoryChunkBig>(
					this, bucketSize * size).release();
				uptr.reset(static_cast<VMMemoryChunkBase*>(rawptr));
			}

			auto ptr = uptr->alloc();

			if (ptr == nullptr)
			{
				assert(false);
				throw std::bad_alloc();
			}
			if (!uptr->full())
				notfullchunks[sizebucket].push_front(uptr.get());

			auto memrange =
				MemoryRange{
					reinterpret_cast<size_t>(uptr->chunk.get()),
					reinterpret_cast<size_t>(uptr->chunk.get() + uptr->memorysize - 1) };
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
		if (dealocFromChunkResult == VMMemoryChunkBase::DealocResult::OK)
		{
			return;
		}
		else if (dealocFromChunkResult == VMMemoryChunkBase::DealocResult::NOW_IS_EMPTY_CHUNK)
		{
			emptyBlocksSize += chunkIt->second->memorysize;
			emptyBlocksCount += 1;
			clean();

			return;
		}
		else if (dealocFromChunkResult == VMMemoryChunkBase::DealocResult::WAS_FULL_CHUNK)
		{
			auto elemsize = chunkIt->second->elSize();
			auto bucketIt = elementSize_perChunk_Map.find(MemoryRange{ elemsize, elemsize });
			if (bucketIt == elementSize_perChunk_Map.end())
			{
				assert(false);
				return;
			}
			
			notfullchunks[bucketIt->first].push_front(chunkIt->second.get());
			return;
		}
		else if (dealocFromChunkResult == VMMemoryChunkBase::DealocResult::WAS_FULL_IS_EMPTY)
		{
			auto elemsize = chunkIt->second->elSize();
			auto bucketIt = elementSize_perChunk_Map.find(MemoryRange{ elemsize, elemsize });
			if (bucketIt == elementSize_perChunk_Map.end())
			{
				assert(false);
				return;
			}

			notfullchunks[bucketIt->first].push_front(chunkIt->second.get());

			emptyBlocksSize += chunkIt->second->memorysize;
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

};

