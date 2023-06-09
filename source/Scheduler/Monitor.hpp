//
//  Monitor.hpp
//  This file is part of the "Scheduler" project and released under the MIT License.
//
//  Created by Samuel Williams on 8/5/2018.
//  Copyright, 2018, by Samuel Williams. All rights reserved.
//

#pragma once

#include "Reactor.hpp"

namespace Scheduler
{
	class Monitor final
	{
	public:
		Monitor(Descriptor descriptor) : _descriptor(descriptor) {}
		
		enum Event : int16_t {
			// No events. When returned, indicates a timeout.
			NONE = 0,
#if defined(SCHEDULER_KQUEUE)
			READABLE = EVFILT_READ,
			WRITABLE = EVFILT_WRITE,
#elif defined(SCHEDULER_EPOLL)
			READABLE = EPOLLIN,
			WRITABLE = EPOLLOUT,
#endif
		};
		
		Event wait(Event events, const Timestamp * timeout = nullptr);
		
		Event wait_readable(const Timestamp * timeout = nullptr);
		Event wait_writable(const Timestamp * timeout = nullptr);
		
	protected:
		Descriptor _descriptor;
	};
}
