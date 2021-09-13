#pragma once
//#include <bitset>
#include <cassert>
#include <memory>
#include <array>
#include <iostream>

enum SplitOptionEnum {

	SOE_Empty = 0,		//non reserved, and not divided
	SOE_Option1 = 1,		//divided for first type
	SOE_Option2 = 2,		//divided for first type
	SOE_Reserved = 3		//reserved
};

struct SplitType {
	uint8_t Option : 2;
	uint8_t fullChilds : 6;
};

constexpr uint32_t shiftLeft(uint32_t a, size_t b) noexcept
{
	return (b <= 32) ? (a << b) : 0;
}

template<size_t SIZE>
struct SplitLeaf 
{
	SplitType state = { SplitOptionEnum::SOE_Empty, 0 };

	size_t tryAllocIndex(size_t _size) noexcept {
		if (empty() && size() == _size)
		{
			setReserved();
			return 0;
		}
		else
			return size_t(-1);
	}
	size_t allocIndex(size_t _size) noexcept
	{
		return tryAllocIndex(_size);
	}
	size_t getSplitCost(size_t _size) const noexcept {
		return size_t(-1);
	}
	bool possibleSplit(size_t _size) const noexcept {
		return false;
	}
	static constexpr uint32_t getMaxSplit() noexcept
	{
		return shiftLeft(1, size() - 1);// uint32_t(1) << (size() - 1);
	}

	size_t deallocIndex(size_t index) noexcept {
		assert(reserved());
		assert(index == 0);
		setEmpty();
		return size();
	}
	
	static constexpr size_t size() noexcept { return SIZE; }
	

	bool full() const noexcept {
		if (state.Option == SOE_Reserved) 
			return true;
		return false;
	}
	bool empty() const noexcept { return state.Option == SOE_Empty; }
	bool reserved() const noexcept { return state.Option == SOE_Reserved; }
	bool FirstTypeSplit() const noexcept { return state.Option == SOE_Option1; }
	bool SecondTypeSplit() const noexcept { return state.Option == SOE_Option2; }
	
	std::pair<uint32_t, uint32_t> getFreeSizeTypes(const uint32_t maxType) const noexcept
	{
		if (empty())
			return { uint32_t(1) << (size() - 1), 0 };
		return { 0,0 };
	}

	void setEmpty() noexcept {
		assert(fullChilds() == 0);
		state.Option = SOE_Empty;
	}
	void setReserved() noexcept {
		assert(state.Option == SOE_Empty);
		state.Option = SOE_Reserved;
	}
	void setSplitType1() noexcept {
		assert(false);
		assert(state.Option == SOE_Empty);
		state.Option = SOE_Option1;
	}
	void setSplitType2() noexcept {
		assert(false);
		assert(state.Option == SOE_Empty);
		state.Option = SOE_Option2;
	}
	
	size_t fullChilds() const noexcept { return state.fullChilds; }
	size_t incFullChilds()noexcept { 
//		std::cout << "incFullChilds:\t" << size() << "\t" << this << "\n";

		assert((state.Option == SOE_Option1) || (state.Option == SOE_Option2));

		assert(!full());

		assert(state.fullChilds < size());  
		return ++state.fullChilds; 
	}
	size_t decFullChilds()noexcept { 
//		std::cout << "decFullChilds:\t" << size() << "\t" << this << "\n";
		
		assert((state.Option == SOE_Option1) || (state.Option == SOE_Option2));
		
		assert(!full());


		assert(state.fullChilds != 0); 
		return --state.fullChilds; 
	}

	void reset() noexcept {
		state = { 0,0 };
	}

	bool correctnessTest() const noexcept {
		if (fullChilds() != 0) return false;
		if (FirstTypeSplit()) return false;
		if (SecondTypeSplit()) return false;

		return true;
	}


};

template<size_t SIZE, typename o1T, size_t sizeO1, typename o2T, size_t sizeO2>
struct SplitMiddle : public SplitLeaf< SIZE > {
	union {
		std::array<o1T, sizeO1> o1;
		std::array<o2T, sizeO2> o2;
	};

	size_t tryAllocIndex(size_t _size) noexcept
	{
		if (this->reserved())
			return size_t(-1);

		if (this->empty())
		{
			if (this->size() == _size)
			{
				this->setReserved();
				return size_t(0);
			}
			//if too small it can't be divided, 
			//if too big it needs to be divided, 
			//		and this function only 'tries' alloc
			return size_t(-1);
		}
		if (this->FirstTypeSplit())
		{
			if(this->fullChilds() == sizeO1)
				return size_t(-1);

			for (size_t i = 0; i < sizeO1; i++)
			{
				bool wasfull = o1[i].full();
				auto index = o1[i].tryAllocIndex(_size);
				if (index != size_t(-1))
				{
					if(!wasfull && o1[i].full())
						this->incFullChilds();
					return i * o1[i].size() + index;
				}
			}
		}

		if (this->SecondTypeSplit())
		{
			if (this->fullChilds() == sizeO2)
				return size_t(-1);
			for (size_t i = 0; i < sizeO2; i++)
			{
				bool wasfull = o2[i].full();
				auto index = o2[i].tryAllocIndex(_size);
				if (index != size_t(-1))
				{
					if (!wasfull && o2[i].full())
						this->incFullChilds();
					return i * o2[i].size() + index;
				}
			}
		}
		
		return size_t(-1);
	}
	size_t getSplitCost(size_t _size) const noexcept 
	{
		if (this->size() == _size)
		{
			if (this->empty())
				return 0;
			else
				return size_t(-1);
		}
		else if (this->size() < _size)
		{
			return size_t(-1);
		}
		else
		{
			if (this->empty())
			{
				auto cost1 = o1[0].getSplitCost(_size);
				auto cost2 = o2[0].getSplitCost(_size);
				auto cost = (std::min)(cost1, cost2);
				if (cost != size_t(-1))
					return cost + this->size() * this->size();
				if((o1[0].size() == _size) || (o2[0].size() == _size))
					return this->size() * this->size();
				return size_t(-1);
			}
			else if(this->reserved())
				return size_t(-1);
			else if (this->FirstTypeSplit())
			{
				size_t minimalCost = size_t(-1);
				for (size_t i = 0; i < o1.size(); i++)
				{
					size_t cost = o1[i].getSplitCost(_size);
					minimalCost = (std::min)(cost, minimalCost);
				}
				return minimalCost;
			}
			else if (this->SecondTypeSplit())
			{
				size_t minimalCost = size_t(-1);
				for (size_t i = 0; i < o2.size(); i++)
				{
					size_t cost = o2[i].getSplitCost(_size);
					minimalCost = (std::min)(cost, minimalCost);
				}
				return minimalCost;
			}
			else
			{
				assert(false);
			}
		}
		return size_t(-1);
	}
	bool possibleSplit(size_t _size) const noexcept {
		if(this->size() < _size)
			return false;

		if (this->size() == _size)
			return true;

		if (o1[0].size() == _size)
			return true;
		if (o2[0].size() == _size)
			return true;

		return o1[0].possibleSplit(_size) || o2[0].possibleSplit(_size);
	}

	constexpr static uint32_t getMaxSplit() noexcept
	{
		constexpr auto dsalfhask= 
			SplitLeaf<SIZE>::getMaxSplit() | 
			o1T::getMaxSplit() | o2T::getMaxSplit();
		return dsalfhask;
	}

	size_t allocIndex(size_t _size) noexcept
	{
		auto index = tryAllocIndex(_size);
		if (index != size_t(-1)) 
			return index;

		if (this->empty())
		{



			if (o1[0].size() == _size)
				this->setSplitType1();
			else if (o2[0].size() == _size)
				this->setSplitType2();
			else if (o1[0].possibleSplit(_size))
				this->setSplitType1();
			else if (o2[0].possibleSplit(_size))
				this->setSplitType2();
			else
				return size_t(-1);
		}

		if (this->FirstTypeSplit())
		{
			size_t minimalSplitCost = size_t(-1);
			size_t minimalSplitIndex = size_t(-1);

			for (size_t i = 0; i < sizeO1; i++)
			{
				size_t cost = o1[i].getSplitCost(_size);
				if (cost < minimalSplitCost)
				{
					minimalSplitCost = cost;
					minimalSplitIndex = i;
				}
				else if (o1[i].empty() && o1[i].size() == _size && (cost > minimalSplitCost || cost == size_t(-1)))
				{
					minimalSplitCost = 0;
					minimalSplitIndex = i;
					break;
				}
			}
			if (minimalSplitIndex != size_t(-1))
			{
				bool wasfull = o1[minimalSplitIndex].full();
				auto index = o1[minimalSplitIndex].allocIndex(_size) + minimalSplitIndex * o1[minimalSplitIndex].size();
				if (!wasfull && o1[minimalSplitIndex].full())
					this->incFullChilds();
				return index;
			}
		}
		else if(this->SecondTypeSplit())
		{

			size_t minimalSplitCost = size_t(-1);
			size_t minimalSplitIndex = size_t(-1);

			for (size_t i = 0; i < sizeO2; i++)
			{
				size_t cost = o2[i].getSplitCost(_size);
				if (cost < minimalSplitCost)
				{
					minimalSplitCost = cost;
					minimalSplitIndex = i;
				}
				else if (o2[i].empty() && o2[i].size() == _size && (cost > minimalSplitCost || cost == size_t(-1)))
				{
					minimalSplitCost = 0;
					minimalSplitIndex = i;
					break;
				}
			}
			if (minimalSplitIndex != size_t(-1))
			{
				bool wasfull = o2[minimalSplitIndex].full();
				auto index = o2[minimalSplitIndex].allocIndex(_size) + minimalSplitIndex * o2[minimalSplitIndex].size();
				if (!wasfull && o2[minimalSplitIndex].full())
					this->incFullChilds();
				return index;
			}
		}
		return size_t(-1);
	}
	size_t deallocIndex(size_t index) noexcept
	{
		assert(!this->empty());
		if (this->reserved())
		{
			this->setEmpty();
			return this->size();
		}
		else if (this->FirstTypeSplit())
		{
			//assert(false);	//sizeO1 czy o1[0].size()
			size_t childindex = index / o1[0].size();
			size_t localindex = index % o1[0].size();
			//bool wasempty = o1[childindex].empty();
			bool wasfull = o1[childindex].full();
			size_t dealocsize = o1[childindex].deallocIndex(localindex);
			bool isempty = o1[childindex].empty();
			//bool isfull = o1[childindex].full();
			
			if (wasfull)
				this->decFullChilds();

			if (isempty)
			{
				bool allchuldempty = true;
				for (const auto& e : o1)
				{
					if (!e.empty())
					{
						allchuldempty = false;
						break;
					}
				}
				if (allchuldempty)
				{
					this->setEmpty();
				}
			}
			return dealocsize;
		}
		else if (this->SecondTypeSplit())
		{
			//assert(false);	//sizeO2 czy o2[0].size()
			size_t childindex = index / o2[0].size();
			size_t localindex = index % o2[0].size();
			//bool wasempty = o2[childindex].empty();
			bool wasfull = o2[childindex].full();
			size_t dealocsize = o2[childindex].deallocIndex(localindex);
			bool isempty = o2[childindex].empty();

			if(wasfull)
				this->decFullChilds();

			if (isempty)
			{
				bool allchuldempty = true;
				for (const auto& e : o2)
				{
					if (!e.empty())
					{
						allchuldempty = false;
						break;
					}
				}
				if (allchuldempty)
				{
					this->setEmpty();
				}
			}
			return dealocsize;
		}
	}

	std::pair<uint32_t,uint32_t> getFreeSizeTypes(const uint32_t maxType) const noexcept
	{
		std::pair<uint32_t, uint32_t> result = { 0,0 };
		if (this->empty())
		{
			result.first |= (((this->size() - 1) < 32) ? (uint32_t(1) << (this->size() - 1)) : 0);
			for (const auto& e : o1)
			{
				auto res = e.getFreeSizeTypes(maxType);
				result.second |= res.first | res.second;
				if ((result.second & maxType) == maxType) 
					return result;
			}
			for (const auto& e : o2)
			{
				auto res = e.getFreeSizeTypes(maxType);
				result.second |= res.first | res.second;
				if ((result.second & maxType) == maxType) 
					return result;
			}
		}
		if (this->FirstTypeSplit())// || this->empty()
		{
			for (const auto& e : o1)
			{
				auto res = e.getFreeSizeTypes(maxType);
				result.first |= res.first;
				result.second |= res.second;
				if (((result.first & maxType) == maxType) && 
					((result.second & maxType) == maxType)) 
					return result;
			}
		}
		if (this->SecondTypeSplit())// || this->empty()
		{
			for (const auto& e : o2)
			{
				auto res = e.getFreeSizeTypes(maxType);
				result.first |= res.first;
				result.second |= res.second;
				if (((result.first & maxType) == maxType) &&
					((result.second & maxType) == maxType))
					return result;
			}
		}
		return result;
	}

	void setSplitType1() noexcept {
		assert(this->state.Option == SOE_Empty);
		this->state.Option = SOE_Option1;
		for(auto &e: o1) e.reset();
	}
	void setSplitType2() noexcept {
		assert(this->state.Option == SOE_Empty);
		this->state.Option = SOE_Option2;
		for (auto& e : o2) e.reset();
	}

	bool full() const noexcept {
		if (this->state.Option == SOE_Reserved)
			return true;

		if (this->state.Option == SOE_Option1)
		{
			if(this->state.fullChilds == sizeO1)
				return true;
			else
				return false;
			//for (const auto& e : o1)
			//	if (!e.full())
			//		return false;
			//return true;
		}
		else if (this->state.Option == SOE_Option2)
		{
			if (this->state.fullChilds == sizeO2)
				return true;
			else
				return false;
			//for (const auto& e : o2)
			//	if (!e.full())
			//		return false;
			//return true;
		}
		return false;
	}



	bool correctnessTest() const noexcept {
		if (this->empty() || this->reserved()){
			if (this->fullChilds() != 0) 
				return false;

			for (const auto& e : o1)
				if (!e.empty())
					return false;;

			for (const auto& e : o1)
				if (!e.empty())
					return false;

			for (const auto& e : o1)
				if (e.correctnessTest() == false)
					return false;

			for (const auto& e : o2)
				if (e.correctnessTest() == false)
					return false;
		}
		else if (this->FirstTypeSplit()) {
			size_t fullcounter = 0;
			for (const auto& e : o1)
				fullcounter += e.full();
			
			if (fullcounter != this->fullChilds()) 
				return false;
			
			//if (fullcounter == 0)
			//	return false;
			
			for (const auto& e : o1)
				if (e.correctnessTest() == false)
					return false;
		}
		else if (this->SecondTypeSplit()) {
			size_t fullcounter = 0;
			for (const auto& e : o2)
				fullcounter += e.full();

			if (fullcounter != this->fullChilds()) 
				return false;

			//if (fullcounter == 0)
			//	return false;

			for (const auto& e : o2)
				if (e.correctnessTest() == false)
					return false;
		}
		else
			return false;
		

		return true;
	}
};

using Split1 = SplitLeaf<1>;
using Split2 = SplitMiddle<2, Split1, 2, Split1, 2>;
using Split3 = SplitLeaf<3>;
using Split4 = SplitMiddle<4, Split2, 2, Split2, 2>;
using Split5 = SplitLeaf<5>;
using Split6 = SplitMiddle<6, Split3, 2, Split3, 2>;
using Split7 = SplitLeaf<7>;
using Split8 = SplitMiddle<8, Split4, 2, Split4, 2>;
using Split9 = SplitMiddle<9, Split3, 3, Split3, 3>;
using Split10 = SplitMiddle<10, Split5, 2, Split5, 2>;
using Split12 = SplitMiddle<12, Split6, 2, Split6, 2>;
using Split14 = SplitMiddle<14, Split7, 2, Split7, 2>;
using Split15 = SplitMiddle<15, Split5, 3, Split5, 3>;
using Split16 = SplitMiddle<16, Split8, 2, Split8, 2>;
using Split18 = SplitMiddle<18, Split6, 3, Split9, 2>;
using Split20 = SplitLeaf<20>;
using Split21 = SplitMiddle<21, Split7, 3, Split7, 3>;
using Split24 = SplitMiddle<24, Split12, 2, Split12, 2>;
using Split25 = SplitLeaf<25>;
using Split28 = SplitMiddle<28, Split14, 2, Split14, 2>;
using Split30 = SplitMiddle<30, Split10, 3, Split15, 2>;
using Split32 = SplitMiddle<32, Split16, 2, Split16, 2>;
using Split72 = SplitMiddle<72, Split18, 4, Split24, 3>;		//
using Split84 = SplitMiddle<84, Split21, 4, Split28, 3>;		//
using Split100 = SplitMiddle<100, Split20, 5, Split25, 4>;		//


//Split72 aksjdhfkjsd;
//
//constexpr auto asdlkjasdklsdv = aksjdhfkjsd.getMaxSplit();
//constexpr auto qwue72 = Split72::getMaxSplit();
//constexpr auto qwue18 = Split18::getMaxSplit();
//constexpr auto qwue24 = Split24::getMaxSplit();

struct SplitTypes {

	SplitTypes() :d0r0({}) 
	{
	}

	static constexpr size_t roundSizeTable[] = {
			0,
			1,2,3,4,
			5,6,7,8,
			9,10,12,12,
			14,14,15,16,
			18,18,20,20,
			21,24,24,24,
			25,28,28,28,
			30,30,32,32
	};
	static constexpr const size_t* getRoundSizeTable() noexcept
	{
		return roundSizeTable;
	}


	union {
		struct D32R0 {
			std::array < Split32, 16> o1;
		}d32r0;
		struct D84R8 {
			std::array < Split84, 6> o1;
			std::array < Split8, 1> r1;
		}d84r8;
		struct D100R12 {
			std::array < Split100, 5> o1;
			std::array < Split4, 1> r1;
			std::array < Split8, 1> r2;
		}d100r12;
		struct D72R8 {
			std::array < Split72, 7> o1;
			std::array < Split8, 1> r1;
		}d72r8;
		struct D30R2 {
			std::array < Split30, 17> o1;
			std::array < Split2, 1> r1;
		}d30r2;
		struct D0R0 {
			std::array < uint8_t, sizeof(D32R0)> o1;
		}d0r0;
	};
	template <typename T1, size_t size1>
	size_t allocIndex(std::array < T1, size1> &o1, size_t _size)
	{
		for (size_t i = 0; i < o1.size(); i++)
		{
			auto& e = o1[i];
			size_t index = e.tryAllocIndex(_size);
			if (index != size_t(-1))
				return index + i*o1[0].size();
		}

		size_t minimalSplitCost = size_t(-1);
		size_t minimalSplitIndex = size_t(-1);
		for (size_t i = 0; i < o1.size(); i++)
		{
			auto& e = o1[i];
			size_t cost = e.getSplitCost(_size);
			if (cost < minimalSplitCost)
			{
				minimalSplitCost = cost;
				minimalSplitIndex = i;
			}
		}
		if (minimalSplitIndex != size_t(-1))
		{
			return o1[minimalSplitIndex].allocIndex(_size)
				+ minimalSplitIndex* o1[minimalSplitIndex].size();
		}
		return size_t(-1);
	}

	void reset(size_t type) {
		assert((type == 0) || (type == 32) ||
			(type == 84) || (type == 100) ||
			(type == 72) || (type == 30));

		switch (type)
		{
		case 32: 
			for (auto& e : d32r0.o1) e.reset(); 
			break;
		case 84:
			for (auto& e : d84r8.o1) e.reset();
			for (auto& e : d84r8.r1) e.reset();
			break;
		case 100:
			for (auto& e : d100r12.o1) e.reset();
			for (auto& e : d100r12.r1) e.reset();
			for (auto& e : d100r12.r2) e.reset();
			break;
		case 72:
			for (auto& e : d72r8.o1) e.reset();
			for (auto& e : d72r8.r1) e.reset();
			break;
		case 30:
			for (auto& e : d30r2.o1) e.reset();
			for (auto& e : d30r2.r1) e.reset();
			break;
		case 0:
			d0r0.o1 = {};
			break; 
		default:
			assert(false);
		}
	}
	bool needReset(size_t type) const noexcept{
		switch (type){
		case 32:
			for (auto& e : d32r0.o1)
				if (e.state.Option != 0 && e.state.fullChilds != 0)
					return true;
			break;
		case 84:
			for (auto& e : d84r8.o1)
				if (e.state.Option != 0 && e.state.fullChilds != 0)
					return true;
			for (auto& e : d84r8.r1)
				if (e.state.Option != 0 && e.state.fullChilds != 0)
					return true;
			break;
		case 100:
			for (auto& e : d100r12.o1)
				if (e.state.Option != 0 && e.state.fullChilds != 0)
					return true;
			for (auto& e : d100r12.r1)
				if (e.state.Option != 0 && e.state.fullChilds != 0)
					return true;
			for (auto& e : d100r12.r2)
				if (e.state.Option != 0 && e.state.fullChilds != 0)
					return true;
			break;
		case 72:
			for (auto& e : d72r8.o1)
				if (e.state.Option != 0 && e.state.fullChilds != 0)
					return true;
			for (auto& e : d72r8.r1)
				if (e.state.Option != 0 && e.state.fullChilds != 0)
					return true;
			break;
		case 30:
			for (auto& e : d30r2.o1)
				if (e.state.Option != 0 && e.state.fullChilds != 0)
					return true;
			for (auto& e : d30r2.r1)
				if (e.state.Option != 0 && e.state.fullChilds != 0)
					return true;
			break;
		case 0:
			for (auto& e : d0r0.o1)
				if (e != 0)
					return true;
			break;
		default:
			assert(false);
		}
		return false;
	}

	size_t allocIndex(size_t type, size_t size) {
		assert(size > 0 && size <= 32);
		
		size = roundSizeTable[size];

		if (type == 32)
		{
			size_t offset = 0;
			size_t index = allocIndex(d32r0.o1, size);
			if (index != size_t(-1))
				return index + offset;
		}
		else if(type == 84)
		{
			size_t offset=0;
			size_t index = allocIndex(d84r8.o1, size);
			if (index != size_t(-1))
				return index+offset;

			offset += d84r8.o1.size()* d84r8.o1[0].size();
			index = allocIndex(d84r8.r1, size);
			if (index != size_t(-1))
				return index + offset;
		}
		else if (type == 100)
		{
			size_t offset = 0;
			size_t index = allocIndex(d100r12.o1, size);
			if (index != size_t(-1))
				return index + offset;

			offset += d100r12.o1.size() * d100r12.o1[0].size();
			index = allocIndex(d100r12.r1, size);
			if (index != size_t(-1))
				return index + offset;

			offset += d100r12.r1.size() * d100r12.r1[0].size();
			index = allocIndex(d100r12.r2, size);
			if (index != size_t(-1))
				return index + offset;
		}
		else if (type == 72)
		{
			size_t offset = 0;
			size_t index = allocIndex(d72r8.o1, size);
			if (index != size_t(-1))
				return index + offset;


			offset += d72r8.o1.size() * d72r8.o1[0].size();
			index = allocIndex(d72r8.r1, size);
			if (index != size_t(-1))
				return index + offset;
		}
		else if (type == 30)
		{
			size_t offset = 0;
			size_t index = allocIndex(d30r2.o1, size);
			if (index != size_t(-1))
				return index + offset;


			offset += d30r2.o1.size() * d30r2.o1[0].size();
			index = allocIndex(d30r2.r1, size);
			if (index != size_t(-1))
				return index + offset;
		}
		return size_t(-1);
	}

	size_t dealocIndex(size_t type, size_t index)
	{
		assert(index < 512);

		if (type == 32)
		{
			auto& e = d32r0.o1;
			auto elsize = e[0].size();
			auto arrindex = index / elsize;
			return e[arrindex].deallocIndex(index - arrindex * elsize);
		}
		else if (type == 84)
		{
			auto& e = d84r8.o1;
			auto& r = d84r8.r1;
			if (index < e[0].size() * e.size())
			{
				auto elsize = e[0].size();
				auto arrindex = index / elsize;
				return e[arrindex].deallocIndex(index - arrindex * elsize);
			}
			else
			{
				index -= e[0].size() * e.size();
				auto elsize = r[0].size();
				auto arrindex = index / elsize;
				return r[arrindex].deallocIndex(index - arrindex * elsize);
			}
		}
		else if (type == 100)
		{
			auto& e =  d100r12.o1;
			auto& r1 = d100r12.r1;
			auto& r2 = d100r12.r2;
			if (index < e[0].size() * e.size())
			{
				auto elsize = e[0].size();
				auto arrindex = index / elsize;
				return e[arrindex].deallocIndex(index - arrindex * elsize);
			}
			else
			{
				index -= e[0].size() * e.size();
				if (index < r1[0].size() * r1.size())
				{
					auto elsize = r1[0].size();
					auto arrindex = index / elsize;
					return r1[arrindex].deallocIndex(index - arrindex * elsize);
				}
				else
				{
					index -= r1[0].size() * r1.size();
					auto elsize = r2[0].size();
					auto arrindex = index / elsize;
					return r2[arrindex].deallocIndex(index - arrindex * elsize);
				}
			}
		}
		else if (type == 72)
		{
			auto& e = d72r8.o1;
			auto& r = d72r8.r1;
			if (index < e[0].size() * e.size())
			{
				auto elsize = e[0].size();
				auto arrindex = index / elsize;
				return e[arrindex].deallocIndex(index - arrindex * elsize);
			}
			else
			{
				index -= e[0].size() * e.size();
				auto elsize = r[0].size();
				auto arrindex = index / elsize;
				return r[arrindex].deallocIndex(index - arrindex * elsize);
			}
		}
		else if (type == 30)
		{
			auto& e = d30r2.o1;
			auto& r = d30r2.r1;
			if (index < e[0].size() * e.size())
			{
				auto elsize = e[0].size();
				auto arrindex = index / elsize;
				return e[arrindex].deallocIndex(index - arrindex * elsize);
			}
			else
			{
				index -= e[0].size() * e.size();
				auto elsize = r[0].size();
				auto arrindex = index / elsize;
				return r[arrindex].deallocIndex(index - arrindex * elsize);
			}
		}
		else
		{
			assert(false);
		}
	}

	template <typename T1, size_t SIZE>
	void getFreeSizeTypes(const std::array < T1, SIZE> &node, std::pair<uint32_t, uint32_t> &result, const uint32_t maxType) const noexcept
	{
		for (const auto& e : node)
		{
			auto res = e.getFreeSizeTypes(maxType);
			result.first |= res.first;
			result.second |= res.second;
			if (((result.first & maxType) == maxType) || ((result.second & maxType) == maxType))
				return;
		}
	}
	std::pair<uint32_t, uint32_t> getFreeSizeTypes(size_t type, const uint32_t maxType) const noexcept
	{
		std::pair<uint32_t, uint32_t> result = { 0,0 };
		if (type == 32)
		{
			getFreeSizeTypes(d32r0.o1, result,maxType);
		}
		else if (type == 84)
		{
			getFreeSizeTypes(d84r8.o1, result, maxType);
			getFreeSizeTypes(d84r8.r1, result, maxType);
		}
		else if (type == 100)
		{
			getFreeSizeTypes(d100r12.o1, result, maxType);
			getFreeSizeTypes(d100r12.r1, result, maxType);
			getFreeSizeTypes(d100r12.r2, result, maxType);
		}
		else if (type == 72)
		{
			getFreeSizeTypes(d72r8.o1, result, maxType);
			getFreeSizeTypes(d72r8.r1, result, maxType);
		}
		else if (type == 30)
		{
			getFreeSizeTypes(d30r2.o1, result, maxType);
			getFreeSizeTypes(d30r2.r1, result, maxType);
		}
		else
		{
			//assert(false);
		}
		return result;
	}
	
	template <size_t TYPE>
	constexpr static uint32_t getMaxSplit() noexcept
	{
		if constexpr (TYPE == 32)
			return Split32::getMaxSplit();
		else if constexpr (TYPE == 84)
			return Split84::getMaxSplit() | Split8::getMaxSplit();
		else if constexpr (TYPE == 100)
			return Split100::getMaxSplit() | Split4::getMaxSplit() | Split8::getMaxSplit();
		else if constexpr (TYPE == 72)
			return Split72::getMaxSplit() | Split8::getMaxSplit();
		else if constexpr (TYPE == 30)
			return Split30::getMaxSplit() | Split2::getMaxSplit();
		else
			static_assert(false);
	}


	bool correctnessTest(size_t type) const noexcept {
		if (type == 32)
		{
			for(const auto &e:d32r0.o1)	if (!e.correctnessTest()) 
				return false;
		}
		else if (type == 84)
		{
			for (const auto& e : d84r8.o1)	
				if (!e.correctnessTest()) 
					return false;
			for (const auto& e : d84r8.r1)	
				if (!e.correctnessTest())
					return false;
		}
		else if (type == 100)
		{
			for (const auto& e : d100r12.o1)	
				if (!e.correctnessTest()) 
					return false;
			for (const auto& e : d100r12.r1)	
				if (!e.correctnessTest()) 
					return false;
			for (const auto& e : d100r12.r2)	
				if (!e.correctnessTest()) 
					return false;
		}
		else if (type == 72)
		{
			for (const auto& e : d72r8.o1)	
				if (!e.correctnessTest()) 
					return false;
			for (const auto& e : d72r8.r1)	
				if (!e.correctnessTest()) 
					return false;
		}
		else if (type == 30)
		{
			for (const auto& e : d30r2.o1)	
				if (!e.correctnessTest()) 
					return false;
			for (const auto& e : d30r2.r1)	
				if (!e.correctnessTest()) 
					return false;
		}
		else
		{
			assert(false);
			
		}

		return true;
	}
};


constexpr size_t afhaskfhuiycxiu = sizeof(SplitTypes);


class MemoryChunkPool
{
	std::unique_ptr<uint8_t[]> memory;
	SplitTypes chunkTypes;
	size_t blockType = 0;// 0 32, 84, 100, 72, 30

public:
	MemoryChunkPool(size_t type = 0)
	{
		reset(type);
	}

	static constexpr const size_t* getRoundSizeTable() noexcept
	{
		return SplitTypes::getRoundSizeTable();
	}

	uint8_t* alloc(size_t size) {
		//size_t blockcount = size >> 11; // size/2048
		//if (size & 0x7FF)
		//	blockcount++;
		size_t blockcount = size;

		if (blockcount > 32 || blockcount < 1)
		{
			assert(false);
			return nullptr;
		}

		size_t index = chunkTypes.allocIndex(blockType, blockcount);
		if (index == size_t(-1))
			return nullptr;
		assert(index < 512);
		return memory.get() + (index << 11);
	}
	size_t dealloc(uint8_t* ptr) {
		assert((memory.get() <= ptr) && (ptr < memory.get() + 2048*512));
		auto index = (ptr - memory.get())/2048;

		return chunkTypes.dealocIndex(blockType, index);
	}
	std::pair<uint32_t, uint32_t> getFreeSizeTypes(const uint32_t maxType) const noexcept 
	{
		return chunkTypes.getFreeSizeTypes(blockType, maxType);
	}
	static constexpr uint32_t getMaxSplit(size_t type) noexcept
	{
		if (type == 32)
			return SplitTypes::getMaxSplit<32>();
		if (type == 84)
			return SplitTypes::getMaxSplit<84>();
		if (type == 100)
			return SplitTypes::getMaxSplit<100>();
		if (type == 72)
			return SplitTypes::getMaxSplit<72>();
		if (type == 30)
			return SplitTypes::getMaxSplit<30>();
		if (type == 0)
			return 0;
		assert(false);
	}
	uint32_t getMaxSplit() const noexcept {
		return getMaxSplit(blockType);
	}

	bool needReset(size_t type) const noexcept
	{
		assert((type == 0) || (type == 32) ||
			(type == 84) || (type == 100) ||
			(type == 72) || (type == 30));
		if (blockType == type)
			return false;

		if (type == 0)
			return true;

		if ((type != 0) && !memory)
			return true;

		
		
		return chunkTypes.needReset(type);
	}
	bool reset(size_t type) {
		if(chunkTypes.needReset(type))
			chunkTypes.reset(type);
		if ((type != 0) && (!memory))
		{
			try {
				memory = std::make_unique<uint8_t[]>(512 * 2048);
			}
			catch (std::bad_alloc &e) {
				return false;
			}
		}
		blockType = type;

		return true;
	}

	std::pair<size_t, size_t> getRange() const noexcept {
		if (memory)
		{
			return {
				reinterpret_cast<size_t>(memory.get()),
				reinterpret_cast<size_t>(memory.get()) + 512 * 2048 - 1 };
		}
		else
			return { 0,0 };
	}

	bool correctnessTest() const noexcept {
		return chunkTypes.correctnessTest(blockType);
	}
};

class MemoryPool {
	std::unique_ptr < MemoryChunkPool> pool;
	std::pair<uint32_t, uint32_t> freeSizes;
	size_t allocatedSize;
	size_t allocatedcount;

public:

	MemoryPool(size_t type = 0) {
		assert((type == 0) || (type == 32) ||
			(type == 84) || (type == 100) ||
			(type == 72) || (type == 30));
		pool = std::make_unique<MemoryChunkPool>();
		computeFreeSizeTypes(pool->getMaxSplit());
	}

	std::pair<size_t, size_t> getRange() const noexcept {
		if (pool) 
			return pool->getRange();
		return {0,0};
	}

	static constexpr const size_t* getRoundSizeTable() noexcept
	{
		return MemoryChunkPool::getRoundSizeTable();
	}

	uint8_t* alloc(size_t size) {
		auto *ptr = pool->alloc(size);
		if (ptr)
		{
			allocatedSize += pool->getRoundSizeTable()[size];
			allocatedcount++;
		}
		freeSizes = pool->getFreeSizeTypes(pool->getMaxSplit());
		return ptr;
	}
	size_t dealloc(uint8_t* ptr) {
		size_t dealocatedsize = pool->dealloc(ptr);
		allocatedSize -= dealocatedsize;
		//assert(allocatedSize != 0 && allocatedSize != size_t(-1));
		allocatedcount-- ;
		freeSizes = pool->getFreeSizeTypes(pool->getMaxSplit());
		return dealocatedsize;
	}
	void computeFreeSizeTypes(const uint32_t maxType) noexcept
	{
		freeSizes = pool->getFreeSizeTypes(maxType);
	}

	std::pair<uint32_t, uint32_t> getFreeSizes() const noexcept
	{
		return freeSizes;
	}

	bool reset(size_t type) {
		assert((type == 0) || (type == 32) ||
			(type == 84) || (type == 100) ||
			(type == 72) || (type == 30));
		if (pool)
		{
			if (!pool->needReset(type)) 
				return true;;
			auto resetResult = pool->reset(type);
			if (resetResult)
				freeSizes = pool->getFreeSizeTypes(pool->getMaxSplit());
			else
				return false;
		}
		else
		{
			try {
				pool = std::make_unique<MemoryChunkPool>();
			}	
			catch (std::bad_alloc &e)
			{
				return false;
			}
		}
		return true;
	}

	bool correctnessTest() const noexcept {
		return pool->correctnessTest();
	}

	size_t getAllocatedSize() const noexcept { return allocatedSize; }
	size_t getAllocatedCount() const noexcept { return allocatedcount; }
};


	static constexpr int class1size = sizeof(Split1);
	static constexpr int class2size = sizeof(Split2);
	static constexpr int class3size = sizeof(Split32);
	static constexpr int class32size = sizeof(SplitTypes);
	static constexpr int MemoryChunkPoolsize = sizeof(MemoryChunkPool);
	static constexpr int MemoryPoolsize = sizeof(MemoryPool);

