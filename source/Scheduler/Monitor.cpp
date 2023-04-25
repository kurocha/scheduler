//
//  Monitor.cpp
//  This file is part of the "Scheduler" project and released under the MIT License.
//
//  Created by Samuel Williams on 8/5/2018.
//  Copyright, 2018, by Samuel Williams. All rights reserved.
//

#include "Monitor.hpp"
#include "Defer.hpp"

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
		remove();
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
		
		_added = Fiber::current;
		_events = events;
		
		_reactor->append({
			static_cast<uintptr_t>(_descriptor),
			events,
			EV_ADD | EV_CLEAR | EV_ONESHOT | EV_UDATA_SPECIFIC,
			0,
			0,
			(void*)Fiber::current
		}, false);
		
		auto defer_removal = defer([&]{
			remove();
		});
		
		_reactor->transfer();
		
		defer_removal.cancel();
		_added = nullptr;
	}
#elif defined(SCHEDULER_EPOLL)
	void Monitor::wait(Monitor::Event events)
	{
		assert(Fiber::current);
		
		int action = EPOLL_CTL_ADD;
		
		if (_added) {
			action = EPOLL_CTL_MOD;
		} else {
			_added = Fiber::current;
		}
		
		_reactor->append(action, _descriptor, events | EPOLLET | EPOLLONESHOT, (void*)Fiber::current);
		
		_reactor->transfer();
	}
#endif

	void Monitor::remove()
	{
		if (_added) {
			auto added = _added;
			_added = nullptr;
			
#if defined(SCHEDULER_KQUEUE)
			_reactor->append({
				static_cast<uintptr_t>(_descriptor),
				_events,
				EV_DELETE | EV_UDATA_SPECIFIC,
				0,
				0,
				added
			});
#elif defined(SCHEDULER_EPOLL)
			_reactor->append(EPOLL_CTL_DEL, _descriptor, 0, added);
#endif
		}
	}
}
