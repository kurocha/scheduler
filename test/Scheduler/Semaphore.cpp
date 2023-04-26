//
//  Semaphore.cpp
//  This file is part of the "Scheduler" project and released under the .
//
//  Created by Samuel Williams on 26/4/2023.
//  Copyright, 2023, by Samuel Williams. All rights reserved.
//

#include <UnitTest/UnitTest.hpp>

#include <Scheduler/Semaphore.hpp>
#include <Scheduler/Reactor.hpp>
#include <Scheduler/After.hpp>

namespace Scheduler
{
	using namespace UnitTest::Expectations;
	
	UnitTest::Suite SemaphoreTestSuite {
		"Scheduler::Semaphore",
		
		{"it can acquire an uncontended semaphore",
			[](UnitTest::Examiner & examiner) {
				Reactor::Bound bound;
				Semaphore semaphore;
				
				examiner.expect(semaphore.count()).to(be == 1);
				
				Fiber fiber([&](){
					Semaphore::Acquire acquire(semaphore);
					
					examiner.expect(semaphore.count()).to(be == 0);
				});
				
				fiber.transfer();
				
				examiner.expect(semaphore.count()).to(be == 1);
			}
		},
		
		{"it can acquire a contended semaphore",
			[](UnitTest::Examiner & examiner) {
				Reactor::Bound bound;
				Semaphore semaphore;
				std::string order;
				
				examiner.expect(semaphore.count()).to(be == 1);
				
				Fiber first_fiber([&](){
					After after(0.1);
					
					order += 'A';
					
					{
						Semaphore::Acquire acquire(semaphore);
						order += 'B';
						after.wait();
						order += 'D';
						
						examiner.expect(semaphore.count()).to(be == 0);
					}
					
					order += 'F';
				});
				
				first_fiber.transfer();
				
				Fiber second_fiber([&](){
					examiner.expect(semaphore.count()).to(be == 0);
					
					order += 'C';
					{
						Semaphore::Acquire acquire(semaphore);
						order += 'E';
					}
				});
				
				second_fiber.transfer();
				
				bound.reactor.run();
				
				examiner.expect(semaphore.count()).to(be == 1);
				examiner.expect(order).to(be == "ABCDEF");
			}
		}
	};
}
