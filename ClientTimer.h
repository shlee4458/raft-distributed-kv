#ifndef __CLIENT_TIMER_H__
#define __CLIENT_TIMER_H__

#include <chrono>

using namespace std::chrono;

class ClientTimer {
public:
	duration<double, std::micro> sum;
	duration<double, std::micro> max;
	duration<double, std::micro> min;
	int op_count;

	time_point<std::chrono::high_resolution_clock> start_time;
	duration<double, std::micro> elapsed_time;

	ClientTimer();
	void operator = (const ClientTimer &timer) {
		sum = timer.sum;
		max = timer.max;
		min = timer.min;
		op_count = timer.op_count;
		start_time = timer.start_time;
		elapsed_time = timer.elapsed_time;
	}
	void Start();
	void End();
	void EndAndMerge();
	void Merge(ClientTimer timer);
	void PrintStats();
	float GetMeanLatency();
	void GenerateCSV();
};

#endif // end of #ifndef __CLIENT_TIMER_H__
