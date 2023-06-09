//
//  Handle.cpp
//  File file is part of the "Scheduler" project and released under the MIT License.
//
//  Created by Samuel Williams on 1/7/2017.
//  Copyright, 2017, by Samuel Williams. All rights reserved.
//

#include "Handle.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cassert>

#include <system_error>

namespace Scheduler
{
	void update_flags(Descriptor descriptor, int flags, bool set)
	{
		int current_flags = ::fcntl(descriptor, F_GETFL, 0);

		if (current_flags == -1)
			throw std::system_error(errno, std::generic_category(), "fcntl(..., F_GETFL, ...)");

		if (set)
			flags |= current_flags;
		else
			flags = current_flags & ~flags;

		if (::fcntl(descriptor, F_SETFL, flags) == -1)
			throw std::system_error(errno, std::generic_category(), "fcntl(..., F_SETFL, ...)");
	}
	
	Handle::Handle(const Handle & other) : _descriptor(::dup(other._descriptor))
	{
		// std::cerr << "descriptor " << other._descriptor << " was duped to " << _descriptor << std::endl;
		
		if (_descriptor == -1)
			throw std::system_error(errno, std::generic_category(), "dup");
	}
	
	Handle & Handle::operator=(const Handle & other)
	{
		auto result = ::dup2(other._descriptor, _descriptor);
		
		if (result == -1)
			throw std::system_error(errno, std::generic_category(), "dup2");
		
		return *this;
	}
	
	Handle::Handle(Handle && other) : _descriptor(other._descriptor)
	{
		other._descriptor = -1;
	}
	
	Handle & Handle::operator=(Handle && other) {
		close();
		
		_descriptor = other._descriptor;
		other._descriptor = -1;
		
		return *this;
	}
	
	void Handle::close()
	{
		if (_descriptor != -1) {
			auto result = ::close(_descriptor);
			
			// Note that the return value should only be used for diagnostics. In particular close() should not be retried after an EINTR since this may cause a reused descriptor from another thread to be closed.
			_descriptor = -1;
			
			if (result == -1) {
				throw std::system_error(errno, std::generic_category(), "close");
			}
		}
	}
	
	Handle::~Handle() noexcept(false)
	{
		close();
	}
}
