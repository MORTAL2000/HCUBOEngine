//
//  query_performance.cpp
//  MPI_Project
//
//  Created by Gabriele on 14/06/16.
//  Copyright © 2016 Gabriele. All rights reserved.
//
#include <ctime>
#include <hcube/core/query_performance.h>
#ifdef _WIN32
	#include <windows.h>
#else
	#include <sys/time.h>
#endif

namespace hcube
{
	namespace query_performance
	{
		//get time
		long long get_ticks()
		{
#ifdef _WIN32
			long long val = 0;
			QueryPerformanceCounter((LARGE_INTEGER *)&val);
			return val;
#else
			struct timeval time_val;

			gettimeofday(&time_val, NULL);

			return (long long)time_val.tv_sec * (1000 * 1000) + (long long)time_val.tv_usec;
#endif
		}
		//time
		double get_time()
		{
			static double coe = 1.0 / 1000.0;
#ifdef _WIN32
			static long long freq = 0;

			if (freq == 0)
			{
				QueryPerformanceFrequency((LARGE_INTEGER *)&freq);
				coe = 1000.0 / freq;
			}
#endif
			return get_ticks() * coe / 1000.0;
		}

		//time
		double get_time_ms()
		{
			static double coe = 1.0 / 1000.0;
#ifdef _WIN32
			static long long freq = 0;

			if (freq == 0)
			{
				QueryPerformanceFrequency((LARGE_INTEGER *)&freq);
				coe = 1000.0 / freq;
			}
#endif
			return get_ticks() * coe;
		}


	}
}