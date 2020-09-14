#pragma once
#include <cstdint>
#include <cassert>
template <size_t ELEMENT_COUNT, size_t BITS_PER_ELEMENT>
class WastedBytesCounter {


	static constexpr size_t NEEDED_MEMORY_IN_BITS =
		ELEMENT_COUNT * BITS_PER_ELEMENT;

	static constexpr size_t BUFFER_SIZE =
		NEEDED_MEMORY_IN_BITS >> 6;

	static constexpr uint64_t MASK =
		~(uint64_t(-1) << BITS_PER_ELEMENT);

	static constexpr size_t ELEMENTS_PER_BLOCK =
		(64ull / BITS_PER_ELEMENT);

	static constexpr size_t divtoshift() {
		switch (BITS_PER_ELEMENT)
		{
		case 0: assert(false); return 0;
		case 1: return 0;
		case 2: return 1;
		case 4: return 2;
		case 8: return 3;
		case 16: return 4;
		case 32: return 5;
		case 64: return 6;
		case 128: return 7;
		case 256: return 8;
		case 512: return 9;
		case 1024: return 10;
		case 2048: return 11;
		case 4096: return 12;
		case 8192: return 13;
		case 16384: return 14;
		case 32768: return 15;
		case 65536: return 16;

		default:
			assert(false);
		}
		return size_t(-1);
	}

	static constexpr size_t asdsgsf = divtoshift();

	uint64_t wastedBytesCount[BUFFER_SIZE] = {};
public:
	size_t getWastedBytes(size_t index) const noexcept {
		if constexpr (BITS_PER_ELEMENT)
		{//assert(index < memorysize / elementSize);
			size_t blockNumber = index >> (6 - divtoshift());
			uint64_t block = wastedBytesCount[blockNumber];
			size_t inBlockIndex = (index & (ELEMENTS_PER_BLOCK - 1));
			size_t inblockAddress = inBlockIndex << divtoshift();	// inBlockIndex * 2 bits
			uint64_t mask = MASK;
			return (block >> inblockAddress) & mask;
		}
		else
			return size_t(-1);
	}
	void setWastedBytes(uint64_t index, size_t size) noexcept
	{
		if constexpr (BITS_PER_ELEMENT)
		{
			//assert(index < memorysize/ elementSize);
			//assert(size < 4);
			size_t blockNumber = index >> (6 - divtoshift());
			uint64_t block = wastedBytesCount[blockNumber];
			size_t inBlockIndex = (index & (ELEMENTS_PER_BLOCK - 1));
			size_t inblockAddress = inBlockIndex << divtoshift();	// inBlockIndex * 2 bits
			uint64_t mask = MASK << inblockAddress;
			block &= ~mask;	//clean
			block |= mask & (uint64_t(size) << inblockAddress);	//write size;
			wastedBytesCount[blockNumber] = block;
		}
	}
};
