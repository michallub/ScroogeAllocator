#pragma once
#include <atomic>
#include <thread>
class MemorySpinlock
{
	std::atomic_flag flag = {};
public:
	void lock()
	{
		while (flag.test_and_set() == true)
			std::this_thread::yield();
	}
	bool try_lock() {
		return flag.test_and_set() == false;
	}
	void unlock()
	{
		flag.clear();
	}


};

