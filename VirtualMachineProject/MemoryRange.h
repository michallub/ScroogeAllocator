#pragma once
#include <utility>

struct MemoryRange
{
	size_t minv, maxv;
	constexpr bool operator<(const MemoryRange& b) const noexcept
	{
		if (maxv < b.minv)
			return true;
		return false;
	}
	constexpr MemoryRange(size_t minv = 0, size_t maxv = 0) :minv(minv), maxv(maxv) {

	}
	template <typename PTRTYPE>
	constexpr MemoryRange(PTRTYPE* minv = 0, PTRTYPE* maxv = 0) :
		minv(reinterpret_cast<size_t>(minv)), maxv(reinterpret_cast<size_t>(maxv)) {

	}
	constexpr MemoryRange(std::pair<size_t, size_t> range) :minv(range.first), maxv(range.second) {

	}
	template <typename PTRTYPE>
	constexpr MemoryRange(std::pair<PTRTYPE*, PTRTYPE*> range) :
		minv(reinterpret_cast<size_t>(range.first)), maxv(reinterpret_cast<size_t>(range.second)) {

	}
};
