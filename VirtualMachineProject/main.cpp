#include <iostream>
#include <chrono>
#include <iomanip>
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
#include "MemoryAllocatorMalloc.h"

template <size_t SIZE, size_t CACHELINE, size_t WAYS>
struct CacheSimulator {
	static constexpr size_t CACHEBUCKETCOUNT = SIZE / CACHELINE / WAYS;
	std::array<std::array<std::pair<void*, size_t>, WAYS>, CACHEBUCKETCOUNT> cache = {};
	size_t missCounter = 0;;
	size_t hitCounter = 0;
	void access(void* ptr)
	{
		const size_t bucketIndex = (size_t(ptr) / CACHELINE)% CACHEBUCKETCOUNT;
		auto& bucket = cache[bucketIndex];
		
		size_t newaddr = size_t(ptr);
		newaddr /= CACHELINE;
		newaddr /= CACHEBUCKETCOUNT;

		for (auto& e : bucket)
		{
			size_t oldaddr = size_t(e.first);
			oldaddr /= CACHELINE;
			oldaddr /= CACHEBUCKETCOUNT;

			if (oldaddr == newaddr)
			{
				e.second = 0;
				hitCounter++;
				return;
			}
		}
		missCounter++;
		size_t maxvalue = 0;
		size_t maxindex = 0;
		size_t index = 0;
		// make bucket older and find the oldest
		for (auto& e : bucket)
		{
			e.second++;
			if (maxvalue < e.second)
			{
				maxvalue = e.second;
				maxindex = index;
			}
			index++;
		}
		bucket[maxindex] = {ptr, 0};
	}



};


#include <Windows.h>
#include <memoryapi.h>





int main() {

	if constexpr (false)
	{
		SYSTEM_INFO sysinf;
		GetSystemInfo(&sysinf);
		size_t virtsize;// = 1024ull * 1024 * 1024 * 1024 * 64;
		std::cin >> virtsize;
		std::cout << "try to allocate " << virtsize << " bytes...\n";
		void* virtptr = VirtualAlloc(nullptr, virtsize, MEM_RESERVE, PAGE_NOACCESS);
		std::cout << "virtptr: " << virtptr << "\n";

		auto minlargepage = GetLargePageMinimum();

		std::cout << "GetLargePageMinimum: " << GetLargePageMinimum() << "\n";;




		while (1) {
			char op;
			std::cout << "allocate or deallocate? (a/d): ";
			std::cin >> op;
			if ((op == 'a') || (op == 'A'))
			{
				size_t where, size;
				std::cout << "where and size: ";
				std::cin >> where >> size;
				std::cout << "VirtualAlloc: " <<
					VirtualAlloc(
						(uint8_t*)(virtptr)+where * 4096,
						size * 4096,
						MEM_COMMIT,
						PAGE_READWRITE) << "\n";
			}
			else if ((op == 'd') || (op == 'D'))
			{
				size_t where, size;
				std::cout << "where and size: ";
				std::cin >> where >> size;
				std::cout << "VirtualFree: " <<
					VirtualFree(
						(uint8_t*)(virtptr)+where * 4096,
						size * 4096,
						MEM_DECOMMIT) << "\n";
			}
			else if ((op == 'q') || (op == 'Q'))
			{
				break;
			}
		}
	}



	MemoryAllocatorMalloc alloc;



	MemoryAllocatorBase& poolAllocator = alloc;
	MemoryAllocatorBase& internalAllocator = alloc;



	omp_set_num_threads(16);
	using LockType = MemorySpinlock;
	//using LockType = std::mutex;


//	std::atomic_size_t testcounter = 0;
//#pragma omp parallel for
//	for (int64_t i = 0; i < 512; i++)
//		for (int64_t j = 0; j < 100000; j++)
//			testcounter++;
//
//	std::cout << "testcounter: " << testcounter << "\n";;
//	//omp_set_num_threads(1);


	//using MemPoolAllocator = MemoryChunkPoolAllocator;
	using MemPoolAllocator = MemoryPoolAllocatorInterface;
	//MemoryManager<MemPoolAllocator> mmm;
	const size_t TABSIZE = 1024 * 1024 * 128 * 0.125 * 8;
	const size_t ELSIZEMIN = 8;
	const size_t ELSIZESIZE = 1;
	uint64_t SEED = 1234325;

	std::cout << "hello\n";

	size_t randomTestIteration = 1024 * 1.00;

	std::atomic_size_t mysumsize = 0, mymaxsumsize = 0;
	std::atomic_size_t newsumsize = 0, newmaxsumsize = 0;
	std::atomic_size_t counterrrrrr = 0;;
	int aaaaaa;

	std::chrono::steady_clock::time_point t0, t1, t2, t3, t2s, t3s, t2i, t3i, t4, t5, t6, t7, tsu1, tsu2, tpu1, tpu2, tsinfo1, tsinfo2, tpinfo1, tpinfo2, tsu1new, tsu2new, tpu1new, tpu2new;
	t0 = t1 = t2 = t3 = t2s = t3s = t2i = t3i = t4 = t5 = t6 = t7 = tsu1 = tsu2 = std::chrono::steady_clock::now();

	if constexpr (true)
	{
		size_t sum = 0;
		std::cout << "Hello world\n";


		//Sleep(2000);
		//{
		//	std::forward_list<std::array<uint8_t, ELSIZEMIN>> list;//std::malloc, std::free, 
		//	t0 = std::chrono::steady_clock::now();
		//	for (size_t i = 0; i < TABSIZE; i++)
		//		list.push_front(std::array<uint8_t, ELSIZEMIN>({ uint8_t(i) }));
		//	t1 = std::chrono::steady_clock::now();
		//	Sleep(2000);
		//	sum = 0;
		//	t2 = std::chrono::steady_clock::now();
		//	for (size_t j = 0; j < 16; j++)
		//		for (auto& e : list)
		//			sum += e[0];
		//	t3 = std::chrono::steady_clock::now();
		//
		//	Sleep(2000);
		//	t4 = std::chrono::steady_clock::now();
		//}
		//t5 = std::chrono::steady_clock::now();
		//Sleep(2000);
		//auto d1 = t1 - t0;
		//auto d2 = t3 - t2;
		//auto d3 = t5 - t4;
		//
		//std::cout << d1.count() << "\tnew list create\n";
		//std::cout << d2.count() << "\tnew list using\n";
		//std::cout << d3.count() << "\tnew list destroy\n";
		//std::cout << "SUM: " << sum << "\n";

		if constexpr (true)
		{

			{
				size_t nodecounter = 0;
				double diffsum = 0.0;

				std::array<std::pair<size_t, std::pair<double, size_t>>, 8> diffsums =
				{
					std::pair<size_t, std::pair<double, size_t>>{16,{0.0, 0ull}},
					std::pair<size_t, std::pair<double, size_t>>{64,{0.0, 0ull}},
					std::pair<size_t, std::pair<double, size_t>>{256,{0.0, 0ull}},
					std::pair<size_t, std::pair<double, size_t>>{1024,{0.0, 0ull}},
					std::pair<size_t, std::pair<double, size_t>>{4096,{0.0, 0ull}},
					std::pair<size_t, std::pair<double, size_t>>{16384,{0.0, 0ull}},
					std::pair<size_t, std::pair<double, size_t>>{65536,{0.0, 0ull}},
					std::pair<size_t, std::pair<double, size_t>>{size_t(-1),{0.0, 0ull}}
				};

				constexpr size_t cacheL1size = 32 * 1024;
				constexpr size_t cacheL1linesize = 64;
				constexpr size_t cacheL1WayNums = 8;
				constexpr size_t cacheL1linescount = cacheL1size / cacheL1WayNums / cacheL1linesize;
				std::array<size_t, cacheL1linescount> cacheL1usagestats = {};

				constexpr size_t cacheL2size = 256 * 1024;
				constexpr size_t cacheL2linesize = 64;
				constexpr size_t cacheL2WayNums = 8;
				constexpr size_t cacheL2linescount = cacheL2size / cacheL2WayNums / cacheL2linesize;
				std::array<size_t, cacheL2linescount> cacheL2usagestats = {};

				constexpr size_t cacheL3size = 6 * 1024 * 1024;
				constexpr size_t cacheL3linesize = 64;
				constexpr size_t cacheL3WayNums = 12;
				constexpr size_t cacheL3linescount = cacheL3size / cacheL3WayNums / cacheL3linesize;
				std::array<size_t, cacheL3linescount> cacheL3usagestats = {};

				auto cacheL1 = std::make_unique<CacheSimulator<32 * 1024, 64, 8>>();
				auto cacheL2 = std::make_unique < CacheSimulator<256 * 1024, 64, 8>>();
				auto cacheL3 = std::make_unique < CacheSimulator<6 * 1024 * 1024, 64, 12>>();

				Sleep(2000);
				{
					MemoryManagerParallel<LockType> memmng(MemoryManagerArguments(internalAllocator, poolAllocator));
					{

						std::forward_list<std::array<uint8_t, ELSIZEMIN>, MemoryManagerAllocator<std::array<uint8_t, ELSIZEMIN>, LockType>> list(memmng);//
						t0 = std::chrono::steady_clock::now();
						for (size_t i = 0; i < TABSIZE; i++)
							list.push_front(std::array<uint8_t, ELSIZEMIN>({ uint8_t(i) }));
						t1 = std::chrono::steady_clock::now();
						Sleep(2000);
						sum = 13;

						//list.sort([](const auto& e1, const auto& e2) {return ((&e1) < (&e2)); });
						//list.sort([](const auto& e1, const auto& e2) {return (e1[0] < e2[0]); });
						//Sleep(2000);

						t2 = std::chrono::steady_clock::now();

						auto prevptr = (&(list.front()));
#pragma omp parallel for
						for (intptr_t j = 0; j < 16; j++)
						{
							size_t lsum = 0;
							for (auto& e : list)
							{
								//intptr_t currentdiff = std::fabs(intptr_t((&(e)) - prevptr));
								//for (auto& e : diffsums)
								//{
								//	if (currentdiff <= e.first)
								//	{
								//		e.second.first += currentdiff;
								//		e.second.second++;
								//		break;
								//	}
								//}
								//size_t curptr = size_t(&(e));
								//cacheL1usagestats[(curptr / cacheL1linesize) % cacheL1linescount]++;
								//cacheL2usagestats[(curptr / cacheL2linesize) % cacheL2linescount]++;
								//cacheL3usagestats[(curptr / cacheL3linesize) % cacheL3linescount]++;
								//cacheL1->access(&(e));
								//cacheL2->access(&(e));
								//cacheL3->access(&(e));
								//
								//diffsum += std::fabs((&(e)) - prevptr);
								//prevptr = (&(e));
								////e[0] += sum;
								lsum += e[0];
								//++nodecounter;
							}
#pragma omp critical
							sum += lsum;
						}
						t3 = std::chrono::steady_clock::now();

						Sleep(2000);

						t2s = std::chrono::steady_clock::now();
						for (intptr_t j = 0; j < 16; j++)
						{
							size_t lsum = 0;
							for (auto& e : list)
							{
								lsum += e[0];
							}

							sum += lsum;
						}
						t3s = std::chrono::steady_clock::now();
						Sleep(2000);

						t2i = std::chrono::steady_clock::now();
						//for (intptr_t j = 0; j < 16; j++)
						{
							for (auto& e : list)
							{
								auto info = memmng.getInfo(e.data());
								if (info.isAllocated == false)
									std::cout << "ERR";
							}
						}
						t3i = std::chrono::steady_clock::now();
						Sleep(2000);

						t4 = std::chrono::steady_clock::now();
					}
					memmng.clean(true);
					t5 = std::chrono::steady_clock::now();

				}
				//globalMemoryManagerParallelWithPoolAllocInterface.clean(true);
				Sleep(2000);
				auto d1 = t1 - t0;
				auto d2 = t3 - t2;
				auto d2s = t3s - t2s;
				auto d2i = t3i - t2i;
				auto d3 = t5 - t4;

				std::cout << d1.count() << "\tmy list create\n";
				std::cout << d2.count() << "\tmy list using parallel\n";
				std::cout << d2s.count() << "\tmy list using serial\n";
				std::cout << d2i.count() << "\tmy list getInfo\n";
				std::cout << d3.count() << "\tmy list destroy\n";
				std::cout << "SUM: " << sum << "\n";
				//std::cout << "nodecounter: " << nodecounter << "\n";
				//std::cout << "diffsum: " << diffsum << "\n";
				//std::cout << "diffsum/nodecounter: " << (diffsum / nodecounter) << "\n";
				//std::cout << "diffsums:\n";
				//for (auto e : diffsums)
				//{
				//	std::cout 
				//		<< std::setprecision(15) << std::fixed << std::right << e.first << "\t"
				//		<< std::setprecision(15) << std::fixed << std::right << e.second.first << "\t"
				//		<< std::setprecision(15) << std::fixed << std::right << e.second.second << "\t"
				//		<< std::setprecision(15) << std::fixed << std::right << (e.second.first/e.second.second) << "\n";
				//}
				//std::cout << "cache L1 stats:\n";
				//size_t cntr = 0;
				//for (auto e : cacheL1usagestats)
				//{
				//	std::cout << std::setprecision(3) << std::setw(8) << e / (double)nodecounter * cacheL1linescount * 100 << " ";
				//	if (cntr++ % 32 == 31) std::cout << "\n";
				//}
				//std::cout << "cache L1 stats:\n";
				//cntr = 0;
				//for (auto e : cacheL2usagestats)
				//{
				//	std::cout << std::setprecision(3) << std::setw(8) << e / (double)nodecounter * cacheL2linescount * 100 << " ";
				//	if (cntr++ % 32 == 31) std::cout << "\n";
				//}
				//std::cout << "cache L1 stats:\n";
				//cntr = 0;
				//for (auto e : cacheL3usagestats)
				//{
				//	std::cout << std::setprecision(3) << std::setw(8) << e / (double)nodecounter * cacheL3linescount * 100 << " ";
				//	if (cntr++ % 32 == 31) std::cout << "\n";
				//}
				//std::cout << "L1 hit/miss: " << cacheL1->hitCounter << "\t" << cacheL1->missCounter << "\n";
				//std::cout << "L2 hit/miss: " << cacheL2->hitCounter << "\t" << cacheL2->missCounter << "\n";
				//std::cout << "L3 hit/miss: " << cacheL3->hitCounter << "\t" << cacheL3->missCounter << "\n";
			}
			{
				size_t nodecounter = 0;
				double diffsum = 0.0;

				std::array<std::pair<size_t, std::pair<double, size_t>>, 8> diffsums =
				{
					std::pair<size_t, std::pair<double, size_t>>{16,{0.0, 0ull}},
					std::pair<size_t, std::pair<double, size_t>>{64,{0.0, 0ull}},
					std::pair<size_t, std::pair<double, size_t>>{256,{0.0, 0ull}},
					std::pair<size_t, std::pair<double, size_t>>{1024,{0.0, 0ull}},
					std::pair<size_t, std::pair<double, size_t>>{4096,{0.0, 0ull}},
					std::pair<size_t, std::pair<double, size_t>>{16384,{0.0, 0ull}},
					std::pair<size_t, std::pair<double, size_t>>{65536,{0.0, 0ull}},
					std::pair<size_t, std::pair<double, size_t>>{size_t(-1),{0.0, 0ull}}
				};

				constexpr size_t cacheL1size = 32 * 1024;
				constexpr size_t cacheL1linesize = 64;
				constexpr size_t cacheL1WayNums = 8;
				constexpr size_t cacheL1linescount = cacheL1size / cacheL1WayNums / cacheL1linesize;
				std::array<size_t, cacheL1linescount> cacheL1usagestats = {};

				constexpr size_t cacheL2size = 256 * 1024;
				constexpr size_t cacheL2linesize = 64;
				constexpr size_t cacheL2WayNums = 8;
				constexpr size_t cacheL2linescount = cacheL2size / cacheL2WayNums / cacheL2linesize;
				std::array<size_t, cacheL2linescount> cacheL2usagestats = {};

				constexpr size_t cacheL3size = 6 * 1024 * 1024;
				constexpr size_t cacheL3linesize = 64;
				constexpr size_t cacheL3WayNums = 12;
				constexpr size_t cacheL3linescount = cacheL3size / cacheL3WayNums / cacheL3linesize;
				std::array<size_t, cacheL3linescount> cacheL3usagestats = {};

				auto cacheL1 = std::make_unique<CacheSimulator<32 * 1024, 64, 8>>();
				auto cacheL2 = std::make_unique < CacheSimulator<256 * 1024, 64, 8>>();
				auto cacheL3 = std::make_unique < CacheSimulator<6 * 1024 * 1024, 64, 12>>();

				Sleep(2000);
				{
					std::forward_list<std::array<uint8_t, ELSIZEMIN>> list;//std::malloc, std::free, 
					t0 = std::chrono::steady_clock::now();
					for (size_t i = 0; i < TABSIZE; i++)
						list.push_front(std::array<uint8_t, ELSIZEMIN>({ uint8_t(i) }));
					t1 = std::chrono::steady_clock::now();
					Sleep(2000);
					sum = 13;

					//list.sort([](const auto& e1, const auto& e2) {return ((&e1) < (&e2)); });
					//list.sort([](const auto& e1, const auto& e2) {return (e1[0] < e2[0]); });
					//Sleep(2000);

					t2 = std::chrono::steady_clock::now();

					auto prevptr = (&(list.front()));
#pragma omp parallel for
					for (intptr_t j = 0; j < 16; j++)
					{
						size_t lsum = 0;
						for (auto& e : list)
						{
							//intptr_t currentdiff = std::fabs(intptr_t((&(e)) - prevptr));
							//for (auto& e : diffsums)
							//{
							//	if (currentdiff <= e.first)
							//	{
							//		e.second.first += currentdiff;
							//		e.second.second++;
							//		break;
							//	}
							//}
							//size_t curptr = size_t(&(e));
							//cacheL1usagestats[(curptr / cacheL1linesize) % cacheL1linescount]++;
							//cacheL2usagestats[(curptr / cacheL2linesize) % cacheL2linescount]++;
							//cacheL3usagestats[(curptr / cacheL3linesize) % cacheL3linescount]++;
							//cacheL1->access(&(e));
							//cacheL2->access(&(e));
							//cacheL3->access(&(e));
							////std::cout << (&(e)) << "\n";
							//diffsum += std::fabs((&(e)) - prevptr);
							//prevptr = (&(e));
							////e[0] += sum;
							lsum += e[0];
							//++nodecounter;
						}
#pragma omp critical
						sum += lsum;
					}
					t3 = std::chrono::steady_clock::now();

					Sleep(2000);
					t2s = std::chrono::steady_clock::now();
					for (intptr_t j = 0; j < 16; j++)
					{
						size_t lsum = 0;
						for (auto& e : list)
						{
							lsum += e[0];
						}

						sum += lsum;
					}
					t3s = std::chrono::steady_clock::now();
					Sleep(2000);

					t4 = std::chrono::steady_clock::now();
				}
				t5 = std::chrono::steady_clock::now();
				Sleep(2000);
				auto d1 = t1 - t0;
				auto d2 = t3 - t2;
				auto d2s = t3s - t2s;
				auto d3 = t5 - t4;

				std::cout << d1.count() << "\tnew list create\n";
				std::cout << d2.count() << "\tnew list using parallel\n";
				std::cout << d2s.count() << "\tnew list using serial\n";
				std::cout << d3.count() << "\tnew list destroy\n";
				std::cout << "SUM: " << sum << "\n";
				//std::cout << "nodecounter: " << nodecounter << "\n";
				//std::cout << "diffsum: " << diffsum << "\n";
				//std::cout << "diffsum/nodecounter: " << (diffsum/ nodecounter) << "\n";
				//std::cout << "diffsums:\n";
				//for (auto e : diffsums)
				//{
				//	std::cout
				//		<< std::setprecision(15) << std::fixed << std::right << e.first << "\t"
				//		<< std::setprecision(15) << std::fixed << std::right << e.second.first << "\t"
				//		<< std::setprecision(15) << std::fixed << std::right << e.second.second << "\t"
				//		<< std::setprecision(15) << std::fixed << std::right << (e.second.first / e.second.second) << "\n";
				//}
				//std::cout << "cache L1 stats:\n";
				//size_t cntr = 0;
				//for (auto e : cacheL1usagestats)
				//{
				//	std::cout << std::setprecision(3) << std::setw(8) << e / (double)nodecounter * cacheL1linescount * 100 << " ";
				//	if (cntr++ % 32 == 31) std::cout << "\n";
				//}
				//std::cout << "cache L1 stats:\n";
				//cntr = 0;
				//for (auto e : cacheL2usagestats)
				//{
				//	std::cout << std::setprecision(3) << std::setw(8) << e / (double)nodecounter * cacheL2linescount * 100 << " ";
				//	if (cntr++ % 32 == 31) std::cout << "\n";
				//}
				//std::cout << "cache L1 stats:\n";
				//cntr = 0;
				//for (auto e : cacheL3usagestats)
				//{
				//	std::cout << std::setprecision(3) << std::setw(8) << e / (double)nodecounter * cacheL3linescount * 100 << " ";
				//	if (cntr++ % 32 == 31) std::cout << "\n";
				//}
				//std::cout << "L1 hit/miss: " << cacheL1->hitCounter << "\t" << cacheL1->missCounter << "\n";
				//std::cout << "L2 hit/miss: " << cacheL2->hitCounter << "\t" << cacheL2->missCounter << "\n";
				//std::cout << "L3 hit/miss: " << cacheL3->hitCounter << "\t" << cacheL3->missCounter << "\n";
			}
		}
	}

	/**/

	

	std::vector<uint8_t*> ptrs(TABSIZE, 0);


	{
		std::cout << "serial tests:\n";
		{
			std::mt19937_64 rng(SEED);
			std::cout << "BEFORE my allocation\n"; Sleep(2000);// std::cin >> aaaaaa;
			MemoryManagerParallel<LockType> mem(MemoryManagerArguments(internalAllocator, poolAllocator));

			


			
			//ALLOCATION TEST
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
			
			Sleep(2000);

			

			//PARALLEL USING TEST
			{
				size_t gsum = 0;
				tpu1 = std::chrono::steady_clock::now();
				{
					
#pragma omp parallel for
					for (intptr_t j = 0; j < 16; j++)
					{
						size_t lsum = 0;
						for (intptr_t i = 0; i < TABSIZE; i++)
						{
							if (ptrs[i])
								lsum += ptrs[i][0] & j;
						}
#pragma omp critical
						gsum += lsum;
					}
				}
				tpu2 = std::chrono::steady_clock::now();
				std::cout << "gsum = " << gsum << "\n";
			}
			Sleep(2000);
			



			//SERIAL USING TEST
			{
				size_t gsum = 0;
				tsu1 = std::chrono::steady_clock::now();
				{
					for (intptr_t j = 0; j < 16; j++)
					{
						size_t lsum = 0;
						for (intptr_t i = 0; i < TABSIZE; i++)
						{
							if (ptrs[i])
								lsum += ptrs[i][0] & j;
						}
						gsum += lsum;
					}
				}
				tsu2 = std::chrono::steady_clock::now();
				std::cout << "gsum = " << gsum << "\n";
			}
			Sleep(2000);





			//PARALLEL GETINFO TEST
			tpinfo1 = std::chrono::steady_clock::now();
			{
#pragma omp parallel for
				for (intptr_t i = 0; i < TABSIZE; i++)
				{
					auto info = mem.getInfo(ptrs[i]);
					if (info.isAllocated == false)
					{
#pragma omp critical
						std::cout << "ERR";
					}
				}
			}
			tpinfo2 = std::chrono::steady_clock::now();

			Sleep(2000);





			//SERIAL GETINFO TEST
			tsinfo1 = std::chrono::steady_clock::now();
			//for (intptr_t j = 0; j < 16; j++)
			{
				for (auto& e : ptrs)
				{
					auto info = mem.getInfo(e);
					if (info.isAllocated == false)
						std::cout << "ERR";
				}
			}
			tsinfo2 = std::chrono::steady_clock::now();

			Sleep(2000);





			//DEALLOCATION TEST
			std::cout << "BEFORE my deallocation\n"; 
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
	}



	{
		std::mt19937_64 rng(SEED);
		std::cout << "BEFORE new allocation\n"; Sleep(2000); //std::cin >> aaaaaa;





		//ALLOCATION TEST
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

		Sleep(2000);





		//PARALLEL USING TEST
		tpu1new = std::chrono::steady_clock::now();
		{
			size_t gsum = 0;
			{
#pragma omp parallel for
				for (intptr_t j = 0; j < 16; j++)
				{
					size_t lsum = 0;
					for (intptr_t i = 0; i < TABSIZE; i++)
					{
						if (ptrs[i])
							lsum += ptrs[i][0] & j;
					}
#pragma omp critical
					gsum += lsum;
				}
			}
			tpu2new = std::chrono::steady_clock::now();
			std::cout << "gsum = " << gsum << "\n";
		}

		Sleep(2000);





		//SERIAL USING TEST
		{
			size_t gsum = 0;
			tsu1new = std::chrono::steady_clock::now();
			{
				for (intptr_t j = 0; j < 16; j++)
				{
					size_t lsum = 0;
					for (intptr_t i = 0; i < TABSIZE; i++)
					{
						if (ptrs[i])
							lsum += ptrs[i][0] & j;
					}
					gsum += lsum;
				}
			}
			tsu2new = std::chrono::steady_clock::now();
			std::cout << "gsum = " << gsum << "\n";
		}
		Sleep(2000);





		//DEALLOCATION TEST
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
	auto dpu = tpu2 - tpu1;
	auto dsu = tsu2 - tsu1;
	auto dpi = tpinfo2 - tpinfo1;
	auto dsi = tsinfo2 - tsinfo1;
	auto dpunew = tpu2new - tpu1new;
	auto dsunew = tsu2new - tsu1new;




	std::cout << d1.count() << "\tmy alloc\n";
	std::cout << d3.count() << "\tnew\n";

	std::cout << d2.count() << "\tmy disalloc\n";
	std::cout << d4.count() << "\tdelete\n";

	std::cout << dpu.count() << "\tmy parallel usage\n";
	std::cout << dpunew.count() << "\tnew parallel usage\n";

	std::cout << dsu.count() << "\tmy serial usage\n";
	std::cout << dsunew.count() << "\tnew serial usage\n";

	std::cout << dpi.count() << "\tmy parallel getInfo\n";
	std::cout << dsi.count() << "\tmy serial getInfo\n";
	std::cout << "\n";
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
		MemoryManagerParallel<LockType> mem(MemoryManagerArguments(internalAllocator, poolAllocator));
		//MemoryManager<MemPoolAllocator> mem;
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
					counterrrrrr++;
					for (size_t j = 0; j < size; j+=16)
						ptrs[index][j] = i * j;
				}
			}
			for (int i = 0; i < deallocationTries; i++)
			{
				//if (i == 61433)
				//	std::cout << "THIS ITERATION";;
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
		auto cntrptr = &counterrrrrr;
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
					(*cntrptr)++;
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
					(*cntrptr)--;
				}
			}
		}
		t5 = std::chrono::steady_clock::now();
		std::cout << "COUNTER = " << counterrrrrr << "\n";
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