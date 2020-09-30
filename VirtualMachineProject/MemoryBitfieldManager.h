#pragma once
#include <cstdint>
//#include <utility>
#include <cassert>
#include "DealocResult.h"

#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_BitScanForward)
#endif

template <size_t SIZE1>
class MemoryBitfieldManager
{
	size_t findFirstEmptyBit(uint64_t bits) const noexcept
	{
		assert(~bits);
#if defined(__GNU__) || defined(__GNUC__)
			return __builtin_ffsll(~bits)-1;
#elif defined(_MSC_VER) && defined(_WIN64)
		unsigned long index;
		auto is = _BitScanForward64(&index, ~bits);
		assert(is);
		return index;
#elif defined(_MSC_VER) && defined(_WIN32)
		uint32_t lbits = bits;
		uint32_t hbits = bits >> 32;
		unsigned long index;
		auto is1 = _BitScanForward(&index, ~lbits);
		if (is1)
			return index;
		auto is2 = _BitScanForward(&index, ~hbits);
		assert(is);
		return index+32;
#else
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
			64 // 0b1111
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
#endif
	}

	uint64_t& bitfield_1() noexcept { return bitfield3[0]; }
	uint64_t getbitfield_1() const noexcept { return bitfield3[0]; }
	void setbitfield_1(uint64_t value) noexcept { bitfield3[0] = value; }
	
	uint64_t& bitfield_2(size_t index) noexcept { return bitfield3[index + 2]; }
	uint64_t getbitfield_2(size_t index) const noexcept { return bitfield3[index + 2]; }
	void setbitfield_2(size_t index, uint64_t value) noexcept { bitfield3[index + 2] = value; }

	uint64_t& bitfield_empty() noexcept { return bitfield3[1]; }
	uint64_t getbitfield_empty() const noexcept { return bitfield3[1]; }
	void setbitfield_empty(uint64_t value) noexcept { bitfield3[1] = value; }


public:
	uint64_t bitfield3[SIZE1 == 1 ? 1 : SIZE1+2] = {};
	uint64_t bitusedmask = 0xFFFF'FFFF'FFFF'FFFF;


	MemoryBitfieldManager() {
		if constexpr (SIZE1 != 1)
			setbitfield_empty(0xFFFF'FFFF'FFFF'FFFF);
	}
	void setElementCount(size_t elementCount) noexcept {
		
		if constexpr (SIZE1 != 1)
		{
			assert((elementCount & 0x3F) == 0);
			auto bits = elementCount >> 6;
			bitusedmask = 0xFFFF'FFFF'FFFF'FFFF >> (64 - bits);
			assert(bitusedmask != 0);
			//switch (elementCount) {
			//case 4096: bitusedmask = 0xFFFF'FFFF'FFFF'FFFF; break;
			//case 2048: bitusedmask = 0x0000'0000'FFFF'FFFF; break;
			//case 1024: bitusedmask = 0x0000'0000'0000'FFFF; break;
			//case 512:  bitusedmask = 0x0000'0000'0000'00FF; break;
			//case 256:  bitusedmask = 0x0000'0000'0000'000F; break;
			//case 128:  bitusedmask = 0x0000'0000'0000'0003; break;
			//case 64:   bitusedmask = 0x0000'0000'0000'0001; break;
			//default:
			//	assert(false);
			//	bitusedmask = 0x0000'0000'0000'0000;
			//	elementCount = 0;
			//	
			//	break;
			//}
			bitfield_empty() &= bitusedmask;
		}
		else if constexpr (SIZE1 == 1)
		{
			auto bits = elementCount;
			bitusedmask = 0xFFFF'FFFF'FFFF'FFFF >> (64 - bits);
			assert(bitusedmask != 0);
			//switch (elementCount) {
			//case 64: bitusedmask = 0xFFFF'FFFF'FFFF'FFFF; break;
			//case 32: bitusedmask = 0x0000'0000'FFFF'FFFF; break;
			//case 16: bitusedmask = 0x0000'0000'0000'FFFF; break;
			//case 8:  bitusedmask = 0x0000'0000'0000'00FF; break;
			//case 4:  bitusedmask = 0x0000'0000'0000'000F; break;
			//case 2:  bitusedmask = 0x0000'0000'0000'0003; break;
			//case 1:   bitusedmask = 0x0000'0000'0000'0001; break;
			//default:
			//	bitusedmask = 0x0000'0000'0000'0000;
			//	elementCount = 0;
			//	assert(false);
			//	break;
			//}
		}
		else
			assert(false);
		
	}
	bool full() const noexcept { return (getbitfield_1() & bitusedmask) == bitusedmask; }
	size_t AllocIndex()
	{
		assert(!full());

		if constexpr (SIZE1 > 1 && SIZE1 <= 64)
		{
			size_t index1 = findFirstEmptyBit(getbitfield_1());
			size_t index2 = findFirstEmptyBit(getbitfield_2(index1));
			size_t index = (index1 << 6) | index2;

			assert((getbitfield_2(index1) & (1ull << index2)) == 0);

			bitfield_empty() &= ~(1ull << index1);
			bitfield_2(index1) |= (1ull << index2);
			if (getbitfield_2(index1) == 0xFFFF'FFFF'FFFF'FFFF)
			{
				assert((getbitfield_1() & (1ull << index1)) == 0);
				bitfield_1() |= (1ull << index1);
			}
			return index;
		}
		else if constexpr (SIZE1 == 1)
		{
			size_t index = findFirstEmptyBit(getbitfield_1());
			assert(index < 64);
			bitfield_1() |= (1ull << index);
			return index;
		}
		else
		{
			assert(false);
			//throw std::bad_alloc();
		}
		
	}
	DealocResult DeallocIndex(size_t index)
	{
		

		auto wasfull = full(); 
		bool isnowempty = 0;;

		if constexpr (SIZE1 > 1 && SIZE1 <= 64)
		{
			assert(((1ull << (index>>6)) & bitusedmask) != 0);
			size_t index1 = index >> 6;
			size_t index2 = index & 0x3F;
			if (getbitfield_2(index1) == 0xFFFF'FFFF'FFFF'FFFF)
			{
				if ((getbitfield_1() & (1ull << index1)) == 0)
					return DealocResult::DEALOC_NOT_ALLOCATED;
				bitfield_1() &= ~(1ull << index1);
			}
			if ((getbitfield_2(index1) & (1ull << index2)) == 0)
				return DealocResult::DEALOC_NOT_ALLOCATED;
			bitfield_2(index1) &= ~(1ull << index2);
			if (getbitfield_2(index1) == 0)
				bitfield_empty() |= (1ull << index1);
		
			isnowempty = ((getbitfield_empty() & bitusedmask) == bitusedmask);
			assert((getbitfield_1() & getbitfield_empty()) == 0);
		}
		else if constexpr (SIZE1 == 1)
		{
			assert(((1ull << index) & bitusedmask) != 0);
			if ((getbitfield_1() & (1ull << index)) == 0)
			{
				assert(false);
				return DealocResult::DEALOC_NOT_ALLOCATED;
			}
			bitfield_1() &= ~(1ull << index);
			isnowempty = ((getbitfield_1() & bitusedmask) == 0ull);

		}
		else
		{
			assert(false);
			//throw std::bad_alloc();
		}

		if (isnowempty)
			if (wasfull)
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
