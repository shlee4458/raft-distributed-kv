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
	ReplicationRequest(int last_idx, int committed_idx, int leader_id, int op_code, int op_arg1, int op_arg2);
	void SetRepairRequest(int last_idx, int committed_idx, int leader_id);
	int Size();

	int GetLastIdx();
	int GetCommitedIdx();
	int GetLeaderId();
	int GetOpCode();
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
    int op_code;
	int op_arg1;
	int op_arg2;
};

class LeaderInfo {
public:
	LeaderInfo();
	int Size();

	std::string GetIp();
	int GetPort();
	void SetLeaderInfo(std::string ip, int port);

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);
	void ParseIp(std::string ip);

private:
	int ip0, ip1, ip2, ip3;
	int port;
};

#endif // #ifndef __MESSAGES_H__
