#include <iostream>
#include <chrono>
#include <random>
#include <Windows.h>
#include <vector>
#include <list>
#include <forward_list>
//#include <thread>

#include <omp.h>

#include "SimpleAllocator.h"
#include "MemoryManager.h"
#include "MemoryManagerParallel.h"
#include "MemorySpinlock.h"


int main() {
	
	omp_set_num_threads(1);
	using LockType = MemorySpinlock;
	//using LockType = std::mutex;


	//using MemPoolAllocator = MemoryChunkPoolAllocator;
	using MemPoolAllocator = MemoryPoolAllocatorInterface;
MemoryManager<MemPoolAllocator> mmm;
const size_t TABSIZE =  1024 * 1024 * 128 * 0.125 * 1;
	const size_t ELSIZEMIN = 24;
	const size_t ELSIZESIZE =1;
	uint64_t SEED = 1234324;

	std::cout << "hello\n";

	size_t randomTestIteration = 1024*0.25;

	std::atomic_size_t mysumsize = 0, mymaxsumsize = 0;
	std::atomic_size_t newsumsize = 0, newmaxsumsize = 0;
	std::atomic_size_t counterrrrrr = 0;;
	int aaaaaa;

	std::chrono::steady_clock::time_point t0, t1, t2, t3, t4, t5, t6, t7;
	t0 = t1 = t2 = t3 = t4 = t5 = t6 = t7 = std::chrono::steady_clock::now();
/*
	if constexpr(false)
	{
		std::cout << "Hello world\n";
		Sleep(2000);
		{
			std::forward_list<std::array<uint8_t, ELSIZEMIN>, MemoryManagerParallelAllocatorWithPoolAllocInterface<std::array<uint8_t, ELSIZEMIN>>> list;
			for (int i = 0; i < TABSIZE; i++)
				//if (rand() % 2)
				//	list.push_back({ rand() });
				//else
				list.push_front(std::array<uint8_t, ELSIZEMIN>({ }));
			Sleep(2000);
			for (int j = 0; j < 16; j++)
				for (auto& e : list)
					if (rand() == RAND_MAX+1)
						std::cout << e[0] << " ";
			std::cout << "\n";
			Sleep(2000);
		}
		globalMemoryManagerParallelWithPoolAllocInterface.clean(true);
		Sleep(2000);
		////for(int i=0;i<20;i++)
		//{
		//	std::forward_list<std::array<uint8_t, ELSIZEMIN>, MemoryManagerAllocatorWithChunkPoolAlloc<std::array<uint8_t, ELSIZEMIN>>> list;
		//	for (int i = 0; i < TABSIZE; i++)
		//		//if (rand() % 2)
		//		//	list.push_back({ rand() });
		//		//else
		//		list.push_front(std::array<uint8_t, ELSIZEMIN>({  }));
		//	Sleep(2000);
		//	for (auto& e : list)
		//		if (rand() == RAND_MAX+1)
		//			std::cout << e[0] << " ";
		//	std::cout << "\n";
		//	Sleep(2000);
		//}
		//globalMemoryManagerWithChunkPoolAlloc.clean(true);
		Sleep(2000);
		{
			std::forward_list<std::array<uint8_t, ELSIZEMIN>> list;//std::malloc, std::free, 
			for (int i = 0; i < TABSIZE; i++)
				//if (rand() % 2)
				//	list.push_back({ rand() });
				//else
				list.push_front(std::array<uint8_t, ELSIZEMIN>({  }));
			Sleep(2000);
			for (int j = 0; j < 16; j++)
				for (auto& e : list)
					if (rand() == RAND_MAX+1)
						std::cout << e[0] << " ";
			std::cout << "\n";
			Sleep(2000);
		}
		Sleep(2000);
	}
	*/
	std::vector<uint8_t*> ptrs(TABSIZE, 0);

	std::cout << "serial tests:\n";
	
	{
		std::mt19937_64 rng(SEED);
		std::cout << "BEFORE my allocation\n"; Sleep(2000);// std::cin >> aaaaaa;
		//MemoryManagerParallel<MemPoolAllocator, MemPoolAllocator, LockType> mem;
		MemoryManager<MemPoolAllocator> mem;

		t0 = std::chrono::steady_clock::now();
#pragma omp parallel for
		for (intptr_t i = 0; i < TABSIZE; i++)
		{
			size_t size = ELSIZEMIN;// (rng() % ELSIZESIZE + ELSIZEMIN);
			mysumsize += size;
			ptrs[i] = mem.alloc(size);
			//assert(mem.correctnessTest());
			for (size_t j = 0; j < size; j += 16)
				ptrs[i][j] = i * j;
		}
		t1 = std::chrono::steady_clock::now();
	
		std::cout << "COUNTER = " << counterrrrrr << "\n";
		std::cout << "BEFORE my deallocation\n"; Sleep(2000); //std::cin >> aaaaaa;
		t2 = std::chrono::steady_clock::now();
#pragma omp parallel for
		for (intptr_t i = 0; i < ptrs.size(); i++)
		{
			mem.dealloc(ptrs[i]);
			ptrs[i] = nullptr;
		}
		mem.clean(true);
		t3 = std::chrono::steady_clock::now();
		std::cout << "BEFORE destroyng MemoryManager\n"; Sleep(2000); //std::cin >> aaaaaa;
	}
	
	ptrs.resize(0);
	ptrs.shrink_to_fit();
	Sleep(2000);
	ptrs.resize(TABSIZE);
	Sleep(2000);
	
	{
		std::mt19937_64 rng(SEED);
		std::cout << "BEFORE new allocation\n"; Sleep(2000); //std::cin >> aaaaaa;
		
		t4 = std::chrono::steady_clock::now();
#pragma omp parallel for
		for (intptr_t i = 0; i < TABSIZE; i++)
		{
			size_t size = ELSIZEMIN;// (rng() % ELSIZESIZE + ELSIZEMIN);
			newsumsize += size;
			ptrs[i] = new uint8_t[size];
			for (size_t j = 0; j < size; j += 16)
				ptrs[i][j] = i * j;
		}
		t5 = std::chrono::steady_clock::now();
	
		std::cout << "BEFORE new deallocation\n"; Sleep(2000); //std::cin >> aaaaaa;
	
		t6 = std::chrono::steady_clock::now();
#pragma omp parallel for
		for (intptr_t i = 0; i < ptrs.size(); i++)
		{
			delete[] ptrs[i];
			ptrs[i] = nullptr;
		}
	
		t7 = std::chrono::steady_clock::now();
	}

	auto d1 = t1 - t0;
	auto d2 = t3 - t2;
	auto d3 = t5 - t4;
	auto d4 = t7 - t6;

	std::cout << d1.count() << "\tmy alloc\n";
	std::cout << d2.count() << "\tmy disalloc\n";
	std::cout << d3.count() << "\tnew\n";
	std::cout << d4.count() << "\tdelete\n";

	std::cout << mysumsize << "\tmysumsize\n";
	std::cout << newsumsize << "\tnewsumsize\n";
	Sleep(2000);


	ptrs.resize(0);
	ptrs.shrink_to_fit();
	Sleep(2000);
	ptrs.resize(TABSIZE);
	Sleep(2000);

//return 0;
	mysumsize = 0;
	newsumsize = 0;

	
	std::cout << "random tests:\n";
	{
		
		std::cout << "BEFORE my allocation\n"; Sleep(2000);// std::cin >> aaaaaa;
		//MemoryManagerParallel<MemPoolAllocator, MemPoolAllocator, LockType> mem;
		MemoryManager<MemPoolAllocator> mem;
		t0 = std::chrono::steady_clock::now();
#pragma omp parallel for
		for (intptr_t iteration = 0; iteration < randomTestIteration; iteration++)
		{
			auto threadsCount = omp_get_num_threads();
			auto threadID = omp_get_thread_num();
			std::mt19937_64 rng(SEED + iteration);
			size_t allocationTries = 64 * 1024;// 256 + rng() % 4096;
			size_t deallocationTries = 64 * 1024;// 4096 + rng() % 65536;

			for (int i = 0; i < allocationTries; i++)
			{
				size_t index = (rng() % (ptrs.size()/ threadsCount - 1))* threadsCount+ threadID;

				if (ptrs[index] == nullptr)
				{
					size_t size = (rng() % ELSIZESIZE + ELSIZEMIN);
					mysumsize += size;
					//mymaxsumsize = std::max(mysumsize, mymaxsumsize);
					ptrs[index] = mem.alloc(size);
					//assert(mem.correctnessTest());
					//counterrrrrr++;
					for (size_t j = 0; j < size; j+=16)
						ptrs[index][j] = i * j;
				}
			}
			for (int i = 0; i < deallocationTries; i++)
			{
				size_t index = (rng() % (ptrs.size() / threadsCount - 1)) * threadsCount + threadID; //auto index = rng() % ptrs.size();
				if (ptrs[index] != nullptr)
				{
					counterrrrrr--;
					mem.dealloc(ptrs[index]);
					ptrs[index] = nullptr;
				}
			}
		}
		t1 = std::chrono::steady_clock::now();
		std::cout << "COUNTER = " << counterrrrrr << "\n";
		std::cout << "BEFORE my deallocation\n"; Sleep(2000); //std::cin >> aaaaaa;
		t2 = std::chrono::steady_clock::now();
#pragma omp parallel for
		for (intptr_t i = 0; i < ptrs.size(); i++)
		{
			mem.dealloc(ptrs[i]);
			ptrs[i] = nullptr;
		}
		mem.clean(true);
		t3 = std::chrono::steady_clock::now();
		std::cout << "BEFORE destroyng MemoryManager\n"; Sleep(2000); //std::cin >> aaaaaa;
	}

	ptrs.resize(0);
	ptrs.shrink_to_fit();
	Sleep(2000);
	ptrs.resize(TABSIZE);
	Sleep(2000);


	{
		std::mt19937_64 rng(SEED);
		std::cout << "BEFORE new allocation\n"; Sleep(2000); //std::cin >> aaaaaa;
		t4 = std::chrono::steady_clock::now();
#pragma omp parallel for
		for (intptr_t iteration = 0; iteration < randomTestIteration; iteration++)
		{
			auto threadsCount = omp_get_num_threads();
			auto threadID = omp_get_thread_num();
			std::mt19937_64 rng(SEED + iteration);
			size_t allocationTries = 64*1024;// 256 + rng() % 4096;
			size_t deallocationTries = 64*1024;// 4096 + rng() % 65536;

			for (int i = 0; i < allocationTries; i++)
			{
				size_t index = (rng() % (ptrs.size() / threadsCount - 1)) * threadsCount + threadID; //auto index = rng() % ptrs.size();
				if (ptrs[index] == nullptr)
				{
					size_t size = (rng() % ELSIZESIZE + ELSIZEMIN);
					newsumsize += size;
					ptrs[index] = new uint8_t[size];
					for (size_t j = 0; j < size; j+=16)
						ptrs[index][j] = i * j;
				}
			}
			for (int i = 0; i < deallocationTries; i++)
			{
				size_t index = (rng() % (ptrs.size() / threadsCount - 1)) * threadsCount + threadID; //auto index = rng() % ptrs.size();
				if (ptrs[index] != nullptr)
				{
					delete[](ptrs[index]);
					ptrs[index] = nullptr;
				}
			}
		}
		t5 = std::chrono::steady_clock::now();

		std::cout << "BEFORE new deallocation\n"; Sleep(2000); //std::cin >> aaaaaa;

		t6 = std::chrono::steady_clock::now();
#pragma omp parallel for
		for (intptr_t i = 0; i < ptrs.size(); i++)
		{
			delete[] ptrs[i];
			ptrs[i] = nullptr;
		}

		t7 = std::chrono::steady_clock::now();
	}
	Sleep(2000);
	ptrs.resize(0);
	ptrs.shrink_to_fit();
	std::cout << "BEFORE program end\n"; Sleep(2000); //std::cin >> aaaaaa;

	d1 = t1 - t0;
	d2 = t3 - t2;
	d3 = t5 - t4;
	d4 = t7 - t6;

	std::cout << d1.count() << "\tmy alloc\n";
	std::cout << d2.count() << "\tmy disalloc\n";
	std::cout << d3.count() << "\tnew\n";
	std::cout << d4.count() << "\tdelete\n";

	std::cout << mysumsize << "\tmysumsize\n"; 
	std::cout << newsumsize << "\tnewsumsize\n";

	Sleep(2000);
	std::cout << "\n";
	//std::cout << sum << "\n";
	return 0;
}