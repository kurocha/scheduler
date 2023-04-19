//
//  Monitor.cpp
//  This file is part of the "Scheduler" project and released under the MIT License.
//
//  Created by Samuel Williams on 8/5/2018.
//  Copyright, 2018, by Samuel Williams. All rights reserved.
//

#include <UnitTest/UnitTest.hpp>

#include <Scheduler/Fiber.hpp>
#include <Scheduler/Monitor.hpp>
#include "Pipe.hpp"

#include <unistd.h>

#include <iostream>

namespace Scheduler
{
	using namespace Concurrent;
	
	UnitTest::Suite MonitorTestSuite {
		"Scheduler::Monitor",
		
		{"it can wait for reading and writing",
			[](UnitTest::Examiner & examiner) {
				std::string order;
				
				Reactor::Bound bound;
				std::vector<char> buffer(12);
				
				auto pipe = Pipe(true);
				
				Fiber server([&](){
					Monitor monitor(pipe.input);
					
					for (std::size_t i = 0; i < 3; i += 1) {
						auto result = ::read(pipe.input, buffer.data(), buffer.size());
						
						if (result == -1) {
							examiner.expect(errno) == EWOULDBLOCK;
							monitor.wait_readable();
						}
					}
				});
				
				server.resume();
				
				Fiber client([&](){
					Monitor monitor(pipe.output);
					
					for (std::size_t i = 0; i < 3; i += 1) {
						auto result = ::write(pipe.output, "Hello World!", 12);
						
						if (result == -1) {
							examiner.expect(errno) == EWOULDBLOCK;
							monitor.wait_writable();
						}
					}
				});
				
				client.resume();
				
				while (client || server)
					bound.reactor.update(1.0);
			}
		},
	};
}
