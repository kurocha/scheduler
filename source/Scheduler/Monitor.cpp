//
//  Monitor.cpp
//  This file is part of the "Scheduler" project and released under the MIT License.
//
//  Created by Samuel Williams on 8/5/2018.
//  Copyright, 2018, by Samuel Williams. All rights reserved.
//

#include "Monitor.hpp"

#include <Concurrent/Fiber.hpp>

#include <cassert>
#include <iostream>

namespace Scheduler
{
	using namespace Concurrent;
	
	Monitor::Monitor(Descriptor descriptor, Reactor * reactor) : _descriptor(descriptor), _reactor(reactor)
	{
	}
	
	Monitor::~Monitor()
	{
		if (_added) {
#if defined(SCHEDULER_KQUEUE)
			_reactor->append({
				static_cast<uintptr_t>(_descriptor),
				EVFILT_WRITE,
				EV_DELETE,
				0,
				0,
				nullptr
			});
#elif defined(SCHEDULER_EPOLL)
			_reactor->append(EPOLL_CTL_DEL, _descriptor, 0, nullptr);
#endif
		}
	}
	
	void Monitor::wait_readable()
	{
		this->wait(Event::READABLE);
	}
	
	void Monitor::wait_writable()
	{
		this->wait(Event::WRITABLE);
	}
	
#if defined(SCHEDULER_KQUEUE)
	void Monitor::wait(Monitor::Event events)
	{
		assert(Fiber::current);
		
		_added = true;
		
		_reactor->append({
			static_cast<uintptr_t>(_descriptor),
			events,
			EV_ADD | EV_CLEAR | EV_ONESHOT,
			0,
			0,
			(void*)Fiber::current
		}, false);
		
		_reactor->transfer();
		
		std::cerr << "back from transfer status=" << (int)Fiber::current->status() << std::endl;
	}
#elif defined(SCHEDULER_EPOLL)
	void Monitor::wait(Monitor::Event events)
	{
		assert(Fiber::current);
		
		int action = EPOLL_CTL_ADD;
		
		if (_added) {
			action = EPOLL_CTL_MOD;
		} else {
			_added = true;
		}
		
		_reactor->append(action, _descriptor, events | EPOLLET | EPOLLONESHOT, (void*)Fiber::current);
		
		_reactor->transfer();
	}
#endif
}
