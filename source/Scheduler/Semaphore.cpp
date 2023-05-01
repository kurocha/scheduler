//
//  Semaphore.cpp
//  This file is part of the "Scheduler" project and released under the .
//
//  Created by Samuel Williams on 26/4/2023.
//  Copyright, 2023, by Samuel Williams. All rights reserved.
//

#include "Semaphore.hpp"

#include "Reactor.hpp"
#include "Defer.hpp"

#include <iostream>

namespace Scheduler
{
	Semaphore::~Semaphore()
	{
		while (!_waiting.empty()) {
			broadcast();
		}
	}
	
	bool Semaphore::acquire(const Timestamp * timeout)
	{
		while (_count == 0) {
			if (!wait(timeout)) return false;
		}
		
		_count -= 1;
		return true;
	}
	
	void Semaphore::release()
	{
		_count += 1;
		signal();
	}
	
	bool Semaphore::wait(const Timestamp * timeout)
	{
		Fiber * fiber = Fiber::current;
		assert(fiber);
		
		auto iterator = _waiting.insert(_waiting.end(), fiber);
		
		auto defer_cleanup = Defer([&]{
			_waiting.erase(iterator);
		});
		
		assert(Reactor::current);
		auto reactor = Reactor::current;
		
		if (timeout) {
			return !reactor->sleep(fiber, *timeout);
		}
		else {
			reactor->transfer();
			return true;
		}
	}
	
	void Semaphore::broadcast()
	{
		while (!_waiting.empty()) {
			auto fiber = _waiting.front();
			
			assert(Reactor::current);
			Reactor::current->transfer(fiber);
		}
	}
	
	void Semaphore::signal(std::size_t count)
	{
		while (!_waiting.empty() && count) {
			auto fiber = _waiting.front();
			
			count -= 1;
			
			assert(Reactor::current);
			Reactor::current->transfer(fiber);
		}
	}
}
