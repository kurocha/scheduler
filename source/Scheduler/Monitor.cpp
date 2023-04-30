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
	
	Monitor::~Monitor()
	{
		remove();
	}
	
	Monitor::Event Monitor::wait_readable(Timestamp * timeout)
	{
		return this->wait(Event::READABLE, timeout);
	}
	
	Monitor::Event Monitor::wait_writable(Timestamp * timeout)
	{
		return this->wait(Event::WRITABLE, timeout);
	}
			
#if defined(SCHEDULER_KQUEUE)
	void Monitor::wait(Monitor::Event events)
	{
		assert(Fiber::current);
		assert(Reactor::current);
		
		_added = Fiber::current;
		_reactor = Reactor::current;
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
	Monitor::Event Monitor::wait(Event events, Timestamp * timeout)
	{
		assert(Fiber::current);
		assert(Reactor::current);
		
		int action;
		if (_reactor) {
			action = EPOLL_CTL_MOD;
		} else {
			action = EPOLL_CTL_ADD;
		}
		
		Reactor::Registration registration{
			.fiber = Fiber::current,
			.result = -1,
		};
		
		_reactor = Reactor::current;
		_events = events;
		
		_reactor->append(action, _descriptor, events | EPOLLET | EPOLLONESHOT, &registration, timeout);
		
		auto defer_removal = defer([&]{
			remove();
		});
		
		_reactor->transfer();
		
		// If we returned with a valid event, we don't need to remove the monitor.
		if (registration.result)
			defer_removal.cancel();
		
		return Event(registration.result);
	}
#endif
	
	void Monitor::remove()
	{
		if (_reactor) {
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
			_reactor->append(EPOLL_CTL_DEL, _descriptor, 0, nullptr, nullptr);
#endif
			_reactor = nullptr;
			_events = NONE;
		}
	}
}
