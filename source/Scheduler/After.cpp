//
//  After.cpp
//  File file is part of the "Scheduler" project and released under the MIT License.
//
//  Created by Samuel Williams on 29/6/2017.
//  Copyright, 2017, by Samuel Williams. All rights reserved.
//

#include "After.hpp"

#include <Concurrent/Fiber.hpp>

#include <cassert>

#if defined(SCHEDULER_EPOLL)
#include <sys/timerfd.h>
#endif

namespace Scheduler
{
	using namespace Concurrent;
	
	void After::wait()
	{
		auto reactor = Reactor::current;
		assert(reactor);
		
		wait(_duration);
	}
	
	void After::wait(const Timestamp & until)
	{
		assert(Fiber::current);
		
		auto reactor = Reactor::current;
		assert(reactor);
		
		reactor->sleep(Fiber::current, until);
	}
}
