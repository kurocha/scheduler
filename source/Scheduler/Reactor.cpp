//
//  Reactor.cpp
//  File file is part of the "Scheduler" project and released under the MIT License.
//
//  Created by Samuel Williams on 27/6/2017.
//  Copyright, 2017, by Samuel Williams. All rights reserved.
//

#include "Reactor.hpp"

#include <Time/Timeout.hpp>
#include <Concurrent/Fiber.hpp>

#include <errno.h>
#include <system_error>
#include <iostream>

#include <unistd.h>

namespace Scheduler
{
	thread_local Reactor * Reactor::current = nullptr;
	
	std::size_t Reactor::update(Interval duration)
	{
		std::size_t count = transfer_ready();
		count += select(duration);
		return count;
	}
	
	std::size_t Reactor::wait(Interval duration)
	{
		Time::Timeout timeout(duration);
		std::size_t count = 0;
		
		timeout.start();
		
		while (true) {
			auto remaining = timeout.remaining();
			
			if (remaining < Interval(0)) break;
			
			count += update(remaining);
		}
		
		return count;
	}
	
	Reactor::~Reactor()
	{
	}
	
	void Reactor::transfer()
	{
		Fiber::main.transfer();
	}
	
	size_t Reactor::transfer_ready()
	{
		size_t count = 0;
		
		while (!_ready.empty()) {
			auto fiber = _ready.front();
			_ready.pop_front();
			
			count += 1;
			fiber->transfer();
		}
		
		return count;
	}
	
#if defined(SCHEDULER_EPOLL)
	Reactor::Reactor() : _selector(::epoll_create1(EPOLL_CLOEXEC))
	{
		_events.reserve(512);
	}
	
	std::size_t Reactor::select(Interval duration)
	{
		_events.resize(_events.capacity());
		auto result = ::epoll_wait(_selector, _events.data(), _events.size(), duration.as_milliseconds());
		
		// If we are interrupted, return gracefully.
		if (result == -1 && errno == EINTR)
			return 0;
		
		if (result == -1)
			throw std::system_error(errno, std::generic_category(), "epoll_wait");
		
		_events.resize(result);
		
		for (auto & event : _events) {
			auto fiber = reinterpret_cast<Concurrent::Fiber *>(event.data.ptr);
			
			if (fiber != nullptr)
				fiber->transfer();
		}
		
		_events.resize(0);
		
		// If we received the maximum number of events, increase the size of the event buffer.
		if (std::size_t(result) == _events.capacity()) {
			_events.reserve(_events.capacity() * 2);
		}
		
		return result;
	}

	void Reactor::append(int operation, Descriptor descriptor, int events, void * data)
	{
		struct epoll_event event;
		event.events = events;
		event.data.fd = descriptor;
		event.data.ptr = data;
		
		auto result = ::epoll_ctl(_selector, operation, descriptor, &event);
		
		if (result == -1)
			throw std::system_error(errno, std::generic_category(), "epoll_ctl");
	}
	
#elif defined(SCHEDULER_KQUEUE)
	Reactor::Reactor() : _selector(::kqueue())
	{
		_events.reserve(512);
	}
	
	std::string filter_name(int16_t filter) {
		switch (filter) {
			case EVFILT_READ: return "EVFILT_READ";
			case EVFILT_WRITE: return "EVFILT_WRITE";
			case EVFILT_TIMER: return "EVFILT_TIMER";
		}
		
		return "???";
	}
	
	std::string flags_name(uint16_t flags) {
		std::string result = "";
		
		if (flags & EV_ADD) result += "EV_ADD|";
		if (flags & EV_DELETE) result += "EV_DELETE|";
		if (flags & EV_ENABLE) result += "EV_ENABLE|";
		if (flags & EV_DISABLE) result += "EV_DISABLE|";
		if (flags & EV_ONESHOT) result += "EV_ONESHOT|";
		if (flags & EV_CLEAR) result += "EV_CLEAR|";
		if (flags & EV_UDATA_SPECIFIC) result += "EV_UDATA_SPECIFIC|";
		
		if (result.size() > 0)
			result.resize(result.size() - 1);
		
		return result;
	}
	
	std::size_t Reactor::select(Interval duration)
	{
		auto timeout = duration.as_timespec();
		
		// TODO is this slow?
		_events.resize(_events.capacity());
		auto result = kevent(_selector, _changes.data(), _changes.size(), _events.data(), _events.size(), duration < Interval(0) ? nullptr : &timeout);
		
		// std::cerr << "select:kqueue = " << result << " errno = " << errno << std::endl;
		// for (auto & change : _changes) {
		// 	std::cerr << "\tchange " << change.ident << " " << filter_name(change.filter) << " " << flags_name(change.flags) << std::endl;
		// }
		
		// If we are interrupted, return gracefully.
		if (result == -1 && errno == EINTR)
			return 0;
		
		if (result == -1) 
			throw std::system_error(errno, std::generic_category(), "kqueue");
		
		_changes.clear();
		_events.resize(result);
		
		for (auto & event : _events) {
			// std::cerr << "\tfiring " << event.ident << " " << filter_name(event.filter) << " " << flags_name(event.flags) << std::endl;
			
			auto fiber = reinterpret_cast<Concurrent::Fiber *>(event.udata);
			
			if (fiber != nullptr) {
				fiber->transfer();
			}
		}
		
		_events.resize(0);
		
		// If we received the maximum number of events, increase the size of the event buffer.
		if (std::size_t(result) == _events.capacity()) {
			_events.reserve(_events.capacity() * 2);
		}
		
		return result;
	}
	
	void Reactor::append(const struct kevent & event, bool flush)
	{
		_changes.push_back(event);
		
		if (flush) {
			auto result = kevent(_selector, _changes.data(), _changes.size(), nullptr, 0, nullptr);
			
			// std::cerr << "append:kqueue = " << result << " errno = " << errno << std::endl;
			// for (auto & change : _changes) {
			// 	std::cerr << "\tchange " << change.ident << " " << filter_name(change.filter) << " " << flags_name(change.flags) << std::endl;
			// }
			
			_changes.clear();
			
			if (result == -1)
				throw std::system_error(errno, std::generic_category(), "kqueue");
		}
	}
#endif
}
