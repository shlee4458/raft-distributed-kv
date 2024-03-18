#ifndef __MESSAGES_H__
#define __MESSAGES_H__

#include <string>

class CustomerRequest {
private:
	int customer_id;
	int order_number;
	int request_type;

public:
	CustomerRequest();
	void operator = (const CustomerRequest &order) {
		customer_id = order.customer_id;
		order_number = order.order_number;
		request_type = order.request_type;
	}
	void SetRequest(int cid, int order_num, int type);
	int GetCustomerId();
	int GetOrderNumber();
	int GetRequestType();

	int Size();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);

	bool IsValid();

	void Print();
};

class LaptopInfo {
private:
	int customer_id;
	int order_number;
	int request_type;
	int engineer_id;
	int admin_id;

public:
	LaptopInfo();
	
	void operator = (const LaptopInfo &info) {
		customer_id = info.customer_id;
		order_number = info.order_number;
		request_type = info.request_type;
		engineer_id = info.engineer_id;
		admin_id = info.admin_id;
	}
	void SetInfo(int cid, int order_num, int type, int engid, int adminid);
	void CopyRequest(CustomerRequest request);
	void SetEngineerId(int id);
	void SetAdminId(int id);

	int GetCustomerId();
	int GetOrderNumber();
	int GetLaptopType();
	int GetEngineerId();
	int GetAdminId();

	int Size();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);

	bool IsValid();

	void Print();
};

class CustomerRecord {
public:
	CustomerRecord();
	void SetRecord(int id, int ordnum);
	int Size();
	int GetCustomerId();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);

	bool IsValid();

	void Print();

private:
	int customer_id;
	int last_order;
};

class Identifier {
public:
	Identifier();
	int Size();

	int GetIdentifier();
	void SetIdentifier(int identifier);

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);

private:
	int identifier;
};

class ReplicationRequest {
public:
	ReplicationRequest();
	ReplicationRequest(int last_idx, int committed_idx, int leader_id, int term, int op_arg1, int op_arg2);
	void SetRepairRequest(int last_idx, int committed_idx, int leader_id);
	int Size();

	int GetLastIdx();
	int GetCommitedIdx();
	int GetLeaderId();
	int GetTerm();
	int GetArg1();
	int GetArg2();
	bool IsValid();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);
	friend std::ostream& operator<<(std::ostream& os, const ReplicationRequest& req);
private:
    int last_idx;
    int committed_idx;
    int leader_id;
	int term;
	int op_arg1;
	int op_arg2;
};

class LeaderInfo {
public:
	LeaderInfo();
	void SetLeaderInfo(std::string ip, int port);
	int Size();

	std::string GetIp();
	int GetPort();
	
	void Marshal(char *buffer);
	void Unmarshal(char *buffer);
	void ParseIp(std::string ip);

private:
	int ip0, ip1, ip2, ip3;
	int port;
};

class RequestVoteMessage {
public:
	RequestVoteMessage();
	void SetRequestVoteMessage(int id, int current_term, int log_size, int last_term);
	int Size();

	int GetId();
	int GetCurrentTerm();
	int GetLogSize();
	int GetLastTerm();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);

	void Print();

private:
	int id, current_term, log_size, last_term;
};

class RequestVoteResponse {
public:
	RequestVoteResponse();
	void SetRequestVoteResponse(int id, int current_term, int voted);
	int Size();

	int GetId();
	int GetCurrentTerm();
	int GetVoted();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);

	void Print();

private:
	int id, current_term, voted;
};

class LogRequest {
public:
	LogRequest();
	void SetLogRequest(int leader_id, 
					   int current_term, 
					   int prefix_length, 
					   int prefix_term, 
					   int commit_length,
					   int op_term,
					   int op_arg1, 
					   int op_arg2);
	int Size();

	int GetLeaderId();
	int GetCurrentTerm();
	int GetPrefixLength();
	int GetPrefixTerm();
	int GetCommitLength();
	int GetOpTerm();
	int GetOpArg1();
	int GetOpArg2();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);
	friend std::ostream& operator<<(std::ostream& os, const LogRequest& req);
private:
	int leader_id;
	int current_term;
	int prefix_length;
	int prefix_term;
	int commit_length;
	int op_term;
	int op_arg1;
	int op_arg2;
};

class LogResponse {
public:
	LogResponse();
	void SetLogResponse(int follower_id, 
					   int current_term, 
					   int ack, 
					   int success);
	int Size();

	int GetFollowerId();
	int GetCurrentTerm();
	int GetAck();
	int GetSuccess();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);
	friend std::ostream& operator<<(std::ostream& os, const LogRequest& req);

	void Print();
	
private:
	int follower_id;
	int current_term;
	int ack;
	int success;
};

#endif // #ifndef __MESSAGES_H__