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
	
	Monitor::Event Monitor::wait_readable(const Timestamp * timeout)
	{
		return this->wait(Event::READABLE, timeout);
	}
	
	Monitor::Event Monitor::wait_writable(const Timestamp * timeout)
	{
		return this->wait(Event::WRITABLE, timeout);
	}
			
#if defined(SCHEDULER_KQUEUE)
	Monitor::Event Monitor::wait(Event events, const Timestamp * timeout)
	{
		// Only one fiber can be waiting at a time on a monitor.
		assert(_events == NONE);
		_events = events;
		
		assert(Reactor::current);
		_reactor = Reactor::current;
		
		assert(Fiber::current);
		_registration.fiber = Fiber::current;
		_registration.result = 0;
		
		_reactor->append({
			static_cast<uintptr_t>(_descriptor),
			events,
			EV_ADD | EV_CLEAR | EV_ONESHOT | EV_UDATA_SPECIFIC,
			0,
			0,
			&_registration
		}, false);
		
		auto defer_removal = defer([&]{
			remove();
		});
		
		_reactor->transfer();
		
		if (_registration.result) {
			_events = NONE;
			_reactor = nullptr;
			defer_removal.cancel();
		}
		
		return Event(_registration.result);
	}
#elif defined(SCHEDULER_EPOLL)
	Monitor::Event Monitor::wait(Event events, Timestamp * timeout)
	{
		// Only one fiber can be waiting at a time on a monitor.
		assert(_events == NONE);
		
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
			.result = 0,
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
				&_registration
			});
#elif defined(SCHEDULER_EPOLL)
			_reactor->append(EPOLL_CTL_DEL, _descriptor, 0, nullptr, nullptr);
#endif
			_reactor = nullptr;
			_events = NONE;
		}
	}
}
