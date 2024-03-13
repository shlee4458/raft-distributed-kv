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

struct PrimaryAdminRequest {
	LaptopInfo laptop;
	std::promise<LaptopInfo> prom;
	std::shared_ptr<ServerStub> stub;
};

struct IdleAdminRequest {
	ReplicationRequest repl_request;
	std::shared_ptr<ServerStub> stub;
};

class LaptopFactory {
private:
	std::queue<std::shared_ptr<PrimaryAdminRequest>> erq;
	std::queue<std::shared_ptr<IdleAdminRequest>> req;

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

	bool heartbeat = false;

	LaptopInfo GetLaptopInfo(CustomerRequest order, int engineer_id);
	LaptopInfo CreateLaptop(CustomerRequest order, int engineer_id, std::shared_ptr<ServerStub> stub);
	int ReadRecord(int customer_id);

	bool PfaHandler(std::shared_ptr<ServerStub> stub);
	void CustomerHandler(int engineer_id, std::shared_ptr<ServerStub> stub);
	void ServerHandler(std::shared_ptr<ServerStub> stub);
	void CandidateVoteHandler(std::shared_ptr<ServerStub> stub);
	void AppendLogHandler(std::shared_ptr<ServerStub> stub);

	void PrimaryMaintainLog(int customer_id, int order_num, const std::shared_ptr<ServerStub>& stub);
	void IdleMaintainLog(int customer_id, int order_num, int req_last, int req_committed, bool was_primary);

	int GetRandomTimeout();

public:
	void EngineerThread(std::shared_ptr<ServerSocket> socket, 
						int engieer_id, 
						std::shared_ptr<ServerMetadata> metadata);
	void PrimaryAdminThread(int id);
	void IdleAdminThread(int id);
	void TimeoutThread();
};

#endif // end of #ifndef __SERVERTHREAD_H__

