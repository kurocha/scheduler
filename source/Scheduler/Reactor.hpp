//
//  Reactor.hpp
//  File file is part of the "Scheduler" project and released under the MIT License.
//
//  Created by Samuel Williams on 27/6/2017.
//  Copyright, 2017, by Samuel Williams. All rights reserved.
//

#pragma once

#include <stdexcept>

#if defined(__linux__)
	#define SCHEDULER_EPOLL
#elif defined(__MACH__)
	#define SCHEDULER_KQUEUE
#endif

#include <vector>
#include <list>
#include <Time/Interval.hpp>

#if defined(SCHEDULER_EPOLL)
	#include <sys/epoll.h>
#elif defined(SCHEDULER_KQUEUE)
	#include <sys/types.h>
	#include <sys/event.h>
	#include <sys/time.h>
#else
	#error "Unable to determine Scheduler implementation."
#endif

#include "Handle.hpp"
#include "Fiber.hpp"

namespace Scheduler
{
	using Time::Interval;
	
	template <typename Callback>
	struct Defer {
		Callback _callback;
		
		Defer(Callback callback) : _callback(callback) {}
		~Defer() {_callback();}
	};
	
	template <typename Callback>
	Defer<Callback> defer(Callback callback) {
		return Defer<Callback>(callback);
	}
	
	class Reactor final
	{
	public:
		static thread_local Reactor * current;
		struct Bound;

		Reactor();
		~Reactor();
		
		void transfer();
		
		// Run the reactor once, waiting for at most the given duration for events to occur.
		std::size_t update(Interval duration);
		
		/// Invoke update multiple times for the given duration.
		std::size_t wait(Interval duration);
		
		const Handle & handle() const noexcept {return _selector;}
		Handle & handle() noexcept {return _selector;}
		
		template <typename Function>
		void fiber(Function && function)
		{
			auto iterator = _fibers.insert(_fibers.end(), nullptr);
			
			iterator->reset(new Fiber([this, iterator, function = std::forward<Function>(function)]{
				auto defer_erase = defer([this, iterator]{
					// Remove the fiber from the list of fibers:
					_fibers.erase(iterator);
				});
				
				function();
			}));
			
			_ready.push_back(iterator->get());
		}
		
	private:
		Handle _selector;
		
		std::list<std::unique_ptr<Fiber>> _fibers;
		std::list<Fiber *> _ready;
		std::size_t transfer_ready();
		
		std::size_t select(Interval duration);
		
#if defined(SCHEDULER_EPOLL)
	public:
		void append(int operation, Descriptor descriptor, int events, void * data);
		
	private:
		std::vector<struct epoll_event> _events;
#elif defined(SCHEDULER_KQUEUE)
	public:
		void append(const struct kevent & event, bool flush = true);
		
	private:
		std::vector<struct kevent> _changes;
		std::vector<struct kevent> _events;
#endif
	};
	
	struct Reactor::Bound {
		Reactor reactor;
		
		Bound()
		{
			if (Reactor::current != nullptr)
				throw std::runtime_error("Reactor::current is already set!");
			
			Reactor::current = &reactor;
		}
		
		~Bound()
		{
			Reactor::current = nullptr;
		}
	};
}
