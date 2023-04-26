//
//  After.cpp
//  This file is part of the "Scheduler" project and released under the MIT License.
//
//  Created by Samuel Williams on 1/7/2017.
//  Copyright, 2017, by Samuel Williams. All rights reserved.
//

#include <UnitTest/UnitTest.hpp>

#include <Scheduler/Fiber.hpp>
#include <Scheduler/After.hpp>

#include <unistd.h>

namespace Scheduler
{
	UnitTest::Suite AfterTestSuite {
		"Scheduler::After",
		
		{"it can wait for duration",
			[](UnitTest::Examiner & examiner) {
				Reactor::Bound bound;
				
				std::string order;
				
				Fiber fiber([&](){
					order += 'B';
					After event(0.1);
					event.wait();
					order += 'D';
					event.wait();
					order += 'F';
				});
				
				order += 'A';
				fiber.transfer();
				order += 'C';
				bound.reactor.run(0.15);
				order += 'E';
				bound.reactor.run(0.15);
				order += 'G';
				
				examiner.expect(order) == "ABCDEFG";
			}
		},
		
		{"it can stop after a timeout",
			[](UnitTest::Examiner & examiner) {
				Reactor::Bound bound;
				unsigned count = 0;
				
				Fiber work_fiber([&](){
					Fiber::current->annotate("work");
					After event(0.1);
					
					while (true) {
						count += 1;
						event.wait();
					}
				});
				
				work_fiber.transfer();
				
				Fiber timeout_fiber([&](){
					Fiber::current->annotate("timeout");
					After event(0.25);
					
					event.wait();
					
					work_fiber.stop();
				});
				
				timeout_fiber.transfer();
				
				bound.reactor.run();
				
				examiner.expect(count) == 3;
			}
		}
	};
}
