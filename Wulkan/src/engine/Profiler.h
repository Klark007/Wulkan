#pragma once

// define #profiling for the measurements to be actually performed

#ifdef PROFILING

#include <chrono>
#include <iostream>

using std::chrono::microseconds;

// needs to be called once per scope where BEGIN_TRACE / END_TRACE are used
#define INIT_TRACE() \
	std::chrono::high_resolution_clock::time_point current

#define BEGIN_TRACE() \
	 current = std::chrono::high_resolution_clock::now()

#define END_TRACE(text) \
	std::cout << text   \
	<< std::chrono::duration_cast<microseconds>(std::chrono::high_resolution_clock::now() - current).count() \
	<< std::endl

#else

#define INIT_TRACE()
#define BEGIN_TRACE()
#define END_TRACE(text)

#endif // PROFILING
