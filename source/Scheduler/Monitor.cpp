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
		assert(Fiber::current);
		assert(Reactor::current);
		auto reactor = Reactor::current;
		
		Reactor::Registration registration;
		
		reactor->append({
			static_cast<uintptr_t>(_descriptor),
			events,
			EV_ADD | EV_CLEAR | EV_ONESHOT | EV_UDATA_SPECIFIC,
			0,
			0,
			&registration
		}, false);
		
		auto defer_removal = defer([&]{
			reactor->append({
				static_cast<uintptr_t>(_descriptor),
				events,
				EV_DELETE | EV_UDATA_SPECIFIC,
				0,
				0,
				&registration
			});
		});
		
		reactor->transfer();
		
		if (registration.result) {
			defer_removal.cancel();
		}
		
		return Event(registration.result);
	}
#elif defined(SCHEDULER_EPOLL)
	Monitor::Event Monitor::wait(Event events, const Timestamp * timeout)
	{
		assert(Fiber::current);
		assert(Reactor::current);
		auto reactor = Reactor::current;
		
		Reactor::Registration registration;
		
		reactor->append(EPOLL_CTL_ADD, _descriptor, events | EPOLLET | EPOLLONESHOT, &registration, timeout);
		
		auto defer_removal = defer([&]{
			reactor->append(EPOLL_CTL_DEL, _descriptor, 0, nullptr, nullptr);
		});
		
		reactor->transfer();
		
		return Event(registration.result);
	}
#endif
}
