//
//  Handle.hpp
//  File file is part of the "Scheduler" project and released under the MIT License.
//
//  Created by Samuel Williams on 1/7/2017.
//  Copyright, 2017, by Samuel Williams. All rights reserved.
//

#pragma once

namespace Scheduler
{
	typedef int Descriptor;
	
	void update_flags(Descriptor descriptor, int flags, bool set = true);
	
	// void set_flags(Descriptor descriptor, int flags, bool value = true);
	// 
	// void set_non_blocking(Descriptor descriptor, bool value = true);
	// void set_close_on_exec(Descriptor descriptor, bool value = true);
	
	// Takes ownership of a descriptor and closes it when it goes out of scope.
	class Handle
	{
	public:
		Handle() {}
		
		// Takes ownership of the descriptor.
		Handle(Descriptor descriptor) {_descriptor = descriptor;}
		
		// Calls close on the descriptor.
		~Handle() noexcept(false);
		
		// Dup the descriptor into the handle.
		Handle(const Handle & other);
		Handle & operator=(const Handle & other);
		
		// Move the descriptor into this handle.
		Handle(Handle && other);
		Handle & operator=(Handle && other);
		
		operator Descriptor() const {return _descriptor;}
		
		void close();
		
		explicit operator bool() const {return _descriptor != -1;}
		
	protected:
		Descriptor _descriptor = -1;
	};
}
