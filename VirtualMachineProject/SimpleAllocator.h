#pragma once
//#include <cstdlib>
#include <limits>
#include <new>
//#include <memory>
#include <iostream>


template <typename T>//void* (*Alloc)(std::size_t), void(*Dealloc)(void*), 
struct SimpleAllocator
{
	typedef T value_type;

	SimpleAllocator() = default;

	template <class U>
	constexpr SimpleAllocator(const SimpleAllocator<U>&) noexcept {}//Alloc, Dealloc, 

	T* allocate(std::size_t n) {
		auto maximum = (std::numeric_limits<int>::max)();
		if (n > maximum / sizeof(T))
			throw std::bad_array_new_length();

		//allocCount++;
		//allocSize += n* sizeof(T); 
		//if (allocCount % 10000==1)
		//	std::cout << "SimpleAllocator::allocate: " << allocCount << "\t" << allocSize << "\t" << n << "\n";

		try {
			return (T*)malloc(n * sizeof(T));//return (T*)Alloc(n * sizeof(T));
		}
		catch (...)	{
		}

		

		throw std::bad_alloc();
	}

	void deallocate(T* ptr, std::size_t n=0) noexcept {

		//if (allocCount % 10000==1)
		//	std::cout << "SimpleAllocator::deallocate: " << allocCount << "\t" << allocSize << "\t" << n << "\n";
		//allocCount--;
		//allocSize -= n * sizeof(T);
		free((void*)ptr);//Dealloc((void*)ptr);
	}


	//static size_t allocCount;
	//static size_t allocSize;

};

template <typename T, typename U> //void* (*Alloc)(std::size_t), void(*Dealloc)(void*), 
bool operator==(const SimpleAllocator <T>&, const SimpleAllocator <U>&) { return true; }//Alloc, Dealloc, Alloc, Dealloc, 

template <typename T, typename U> //void* (*Alloc)(std::size_t), void(*Dealloc)(void*), 
bool operator!=(const SimpleAllocator <T>&, const SimpleAllocator <U>&) { return false; }//Alloc, Dealloc, Alloc, Dealloc, 

//template <typename T>
//size_t SimpleAllocator<T>::allocCount = 0;
//template <typename T>
//size_t SimpleAllocator<T>::allocSize = 0;
