#include <chrono>
#include <iostream>
#include <vector>
using namespace std::chrono;

struct timer
{
	steady_clock::time_point start_timer;
	steady_clock::time_point end_timer;
	duration<double> elapsed_time;
	std::vector<int> time_values;

	void start()
	{
		start_timer = steady_clock::now();
	}

	void end()
	{
		end_timer = steady_clock::now();
	}

	double measure_time()
	{
		elapsed_time = end_timer - start_timer;
		double elapsed_microsecs = duration_cast<microseconds>(elapsed_time).count();
		time_values.push_back(elapsed_microsecs);
		return elapsed_microsecs;
	}

	double get_average_time()
	{
		int average = 0;
		for (int i = 0; i = size(time_values); i++)
		{
			average += average + time_values[i];
		}
		return average / size(time_values);
	}

};