#ifndef __SERVERTHREAD_H__
#define __SERVERTHREAD_H__

#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <vector>
#include <thread>
#include <map>
#include <deque>

#include "Messages.h"
#include "ServerStub.h"
#include "ServerSocket.h"
#include "ServerMetadata.h"

struct LeaderRequest {
	LaptopInfo laptop;
	std::promise<LaptopInfo> prom;
	std::shared_ptr<ServerStub> stub;
};

struct FollowerRequest {
	LogRequest log_request;
	std::shared_ptr<ServerStub> stub;
};

class LaptopFactory {
private:
	std::queue<std::shared_ptr<LeaderRequest>> erq;
	std::queue<std::shared_ptr<FollowerRequest>> req;

	std::mutex erq_lock;
	std::mutex rep_lock;
	std::mutex stub_lock;
	std::mutex meta_lock;
	std::mutex timeout_lock;

	std::condition_variable erq_cv;
	std::condition_variable rep_cv;
	std::condition_variable timeout_cv;

	std::shared_ptr<ServerMetadata> metadata;
	std::vector<std::shared_ptr<ServerStub>> stubs;

	LaptopInfo GetLaptopInfo(CustomerRequest order, int engineer_id);
	LaptopInfo CreateLaptop(CustomerRequest order, int engineer_id, std::shared_ptr<ServerStub> stub);
	int ReadRecord(int customer_id);

	void CustomerHandler(int engineer_id, std::shared_ptr<ServerStub> stub);
	void ServerHandler(std::shared_ptr<ServerStub> stub);
	void CandidateVoteHandler(std::shared_ptr<ServerStub> stub);
	int AppendLogHandler(std::shared_ptr<ServerStub> stub);

	void LeaderMaintainLog(int customer_id, int order_num, const std::shared_ptr<ServerStub>& stub);
	void FollowerMaintainLog(int customer_id, int order_num, int req_last, int req_committed, bool was_primary);

	int GetRandomTimeout();

public:
	LaptopFactory(std::shared_ptr<ServerMetadata> metadata);
	void EngineerThread(std::shared_ptr<ServerSocket> socket, 
						int engieer_id);
	void LeaderThread(int id);
	void TimeoutThread();
	void FollowerThread();
};

#endif // end of #ifndef __SERVERTHREAD_H__

