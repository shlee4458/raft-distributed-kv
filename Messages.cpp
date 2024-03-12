#include <cstring>
#include <iostream>
#include <arpa/inet.h>
#include <vector>

#include "Messages.h"

/**
 * Customer Request
*/

CustomerRequest::CustomerRequest() {
	customer_id = -1;
	order_number = -1;
	request_type = -1;
}

void CustomerRequest::SetRequest(int id, int number, int type) {
	customer_id = id;
	order_number = number;
	request_type = type;
}

int CustomerRequest::GetCustomerId() { return customer_id; }
int CustomerRequest::GetOrderNumber() { return order_number; }
int CustomerRequest::GetRequestType() { return request_type; }

int CustomerRequest::Size() {
	return sizeof(customer_id) + sizeof(order_number) + sizeof(request_type);
}

void CustomerRequest::Marshal(char *buffer) {
	int net_customer_id = htonl(customer_id);
	int net_order_number = htonl(order_number);
	int net_request_type = htonl(request_type);
	int offset = 0;
	memcpy(buffer + offset, &net_customer_id, sizeof(net_customer_id));
	offset += sizeof(net_customer_id);
	memcpy(buffer + offset, &net_order_number, sizeof(net_order_number));
	offset += sizeof(net_order_number);
	memcpy(buffer + offset, &net_request_type, sizeof(net_request_type));
}

void CustomerRequest::Unmarshal(char *buffer) {
	int net_customer_id;
	int net_order_number;
	int net_request_type;
	int offset = 0;
	memcpy(&net_customer_id, buffer + offset, sizeof(net_customer_id));
	offset += sizeof(net_customer_id);
	memcpy(&net_order_number, buffer + offset, sizeof(net_order_number));
	offset += sizeof(net_order_number);
	memcpy(&net_request_type, buffer + offset, sizeof(net_request_type));

	customer_id = ntohl(net_customer_id);
	order_number = ntohl(net_order_number);
	request_type = ntohl(net_request_type);
}

bool CustomerRequest::IsValid() {
	return (customer_id != -1);
}

void CustomerRequest::Print() {
	std::cout << "id " << customer_id << " ";
	std::cout << "num " << order_number << " ";
	std::cout << "type " << request_type << std::endl;
}


/**
 * Laptop Info
*/

LaptopInfo::LaptopInfo() {
	customer_id = -1;
	order_number = -1;
	request_type = -1;
	engineer_id = -1;
	admin_id = -1;
}

void LaptopInfo::SetInfo(int id, int number, int type, int engid, int adminid) {
	customer_id = id;
	order_number = number;
	request_type = type;
	engineer_id = engid;
	admin_id = adminid;
}

void LaptopInfo::CopyRequest(CustomerRequest order) {
	customer_id = order.GetCustomerId();
	order_number = order.GetOrderNumber();
	request_type = order.GetRequestType();
}

void LaptopInfo::SetEngineerId(int id) { engineer_id = id; }
void LaptopInfo::SetAdminId(int id) { admin_id = id; }

int LaptopInfo::GetCustomerId() { return customer_id; }
int LaptopInfo::GetOrderNumber() { return order_number; }
int LaptopInfo::GetLaptopType() { return request_type; }
int LaptopInfo::GetEngineerId() { return engineer_id; }
int LaptopInfo::GetAdminId() { return admin_id; }

int LaptopInfo::Size() {
	return sizeof(customer_id) + sizeof(order_number) + sizeof(request_type)
		+ sizeof(engineer_id) + sizeof(admin_id);
}

void LaptopInfo::Marshal(char *buffer) {
	int net_customer_id = htonl(customer_id);
	int net_order_number = htonl(order_number);
	int net_request_type = htonl(request_type);
	int net_engineer_id = htonl(engineer_id);
	int net_expert_id = htonl(admin_id);
	int offset = 0;

	memcpy(buffer + offset, &net_customer_id, sizeof(net_customer_id));
	offset += sizeof(net_customer_id);
	memcpy(buffer + offset, &net_order_number, sizeof(net_order_number));
	offset += sizeof(net_order_number);
	memcpy(buffer + offset, &net_request_type, sizeof(net_request_type));
	offset += sizeof(net_request_type);
	memcpy(buffer + offset, &net_engineer_id, sizeof(net_engineer_id));
	offset += sizeof(net_engineer_id);
	memcpy(buffer + offset, &net_expert_id, sizeof(net_expert_id));

}

void LaptopInfo::Unmarshal(char *buffer) {
	int net_customer_id;
	int net_order_number;
	int net_request_type;
	int net_engineer_id;
	int net_expert_id;
	int offset = 0;

	memcpy(&net_customer_id, buffer + offset, sizeof(net_customer_id));
	offset += sizeof(net_customer_id);
	memcpy(&net_order_number, buffer + offset, sizeof(net_order_number));
	offset += sizeof(net_order_number);
	memcpy(&net_request_type, buffer + offset, sizeof(net_request_type));
	offset += sizeof(net_request_type);
	memcpy(&net_engineer_id, buffer + offset, sizeof(net_engineer_id));
	offset += sizeof(net_engineer_id);
	memcpy(&net_expert_id, buffer + offset, sizeof(net_expert_id));

	customer_id = ntohl(net_customer_id);
	order_number = ntohl(net_order_number);
	request_type = ntohl(net_request_type);
	engineer_id = ntohl(net_engineer_id);
	admin_id = ntohl(net_expert_id);
}

bool LaptopInfo::IsValid() {
	return (customer_id != -1);
}

void LaptopInfo::Print() {
	std::cout << "id " << customer_id << " ";
	std::cout << "num " << order_number << " ";
	std::cout << "type " << request_type << " ";
	std::cout << "engid " << engineer_id << " ";
	std::cout << "expid " << admin_id << std::endl;
}

/**
 * CustomerRecord
*/

CustomerRecord::CustomerRecord() {
	customer_id = -2;
	last_order = -1;
}

bool CustomerRecord::IsValid() {
	return last_order != -1;
}

int CustomerRecord::GetCustomerId() {
	return customer_id;
}

void CustomerRecord::SetRecord(int id, int ordnum) {
	customer_id = id;
	last_order = ordnum;
}

int CustomerRecord::Size() {
	return sizeof(customer_id) + sizeof(last_order);
}

void CustomerRecord::Marshal(char *buffer) {
	int net_customer_id = htonl(customer_id);
	int net_last_order = htonl(last_order);
	int offset = 0;
	memcpy(buffer + offset, &net_customer_id, sizeof(net_customer_id));
	offset += sizeof(net_customer_id);
	memcpy(buffer + offset, &net_last_order, sizeof(net_last_order));
}

void CustomerRecord::Unmarshal(char *buffer) {
	int net_customer_id;
	int net_last_order;
	int offset = 0;

	memcpy(&net_customer_id, buffer + offset, sizeof(net_customer_id));
	offset += sizeof(net_customer_id);
	memcpy(&net_last_order, buffer + offset, sizeof(net_last_order));
	
	customer_id = ntohl(net_customer_id);
	last_order = ntohl(net_last_order);
}

void CustomerRecord::Print() {
	std::cout << "Customer ID: "<< customer_id << "\t";
	std::cout << "Last Order num: "<< last_order << std::endl;
}

/**
 * Identifier 
*/

Identifier::Identifier()
: identifier(0) {	}

void Identifier::SetIdentifier(int identifier) {
	this->identifier = identifier;
}

int Identifier::Size() {
	return sizeof(identifier);
}

int Identifier::GetIdentifier() {
	return identifier;
}

void Identifier::Marshal(char *buffer) {
	int net_identifer = htonl(identifier);
	memcpy(buffer, &net_identifer, sizeof(net_identifer));
}

void Identifier::Unmarshal(char *buffer) {
	int net_identifer;
	memcpy(&net_identifer, buffer, sizeof(net_identifer));
	identifier = ntohl(net_identifer);
}

/**
 * Replication Message
*/
ReplicationRequest::ReplicationRequest()
:last_idx(-1), committed_idx(-1), leader_id(-1) { }

ReplicationRequest::ReplicationRequest(int last_idx, int committed_idx, int leader_id, int op_code, int op_arg1, int op_arg2) {
    this->last_idx = last_idx;
    this->committed_idx = committed_idx;
    this->leader_id = leader_id;
    this->op_code = op_code;
	this->op_arg1 = op_arg1;
	this->op_arg2 = op_arg2;
}

void ReplicationRequest::SetRepairRequest(int last_idx, int committed_idx, int leader_id) {
    this->last_idx = last_idx;
    this->committed_idx = committed_idx;
    this->leader_id = leader_id;
}

int ReplicationRequest::Size() {
	return sizeof(last_idx) + sizeof(committed_idx) + sizeof(leader_id) 
	+ sizeof(op_code) + sizeof(op_arg1) + sizeof(op_arg2);
}

int ReplicationRequest::GetLastIdx() {
	return last_idx;
}
int ReplicationRequest::GetCommitedIdx() {
	return committed_idx;
}
int ReplicationRequest::GetLeaderId() {
	return leader_id;
}
int ReplicationRequest::GetOpCode() {
	return op_code;
}
int ReplicationRequest::GetArg1() {
	return op_arg1;
}
int ReplicationRequest::GetArg2() {
	return op_arg2;
}

bool ReplicationRequest::IsValid() {
	return last_idx != -1;
}

void ReplicationRequest::Marshal(char *buffer) {
	int net_primary_id = htonl(leader_id);
	int net_last_idx = htonl(last_idx);
    int net_committed_idx = htonl(committed_idx);
    int net_opcode = htonl(op_code);
    int net_arg1 = htonl(op_arg1);
    int net_arg2 = htonl(op_arg2);

	int offset = 0;
	memcpy(buffer + offset, &net_primary_id, sizeof(net_primary_id));
	offset += sizeof(net_primary_id);
	memcpy(buffer + offset, &net_last_idx, sizeof(net_last_idx));
	offset += sizeof(net_last_idx);
	memcpy(buffer + offset, &net_committed_idx, sizeof(net_committed_idx));
	offset += sizeof(net_committed_idx);
	memcpy(buffer + offset, &net_opcode, sizeof(net_opcode));
	offset += sizeof(net_opcode);
	memcpy(buffer + offset, &net_arg1, sizeof(net_arg1));
    offset += sizeof(net_arg1);
	memcpy(buffer + offset, &net_arg2, sizeof(net_arg2));
}

void ReplicationRequest::Unmarshal(char *buffer) {
	int net_primary_id;
	int net_last_idx;
    int net_committed_idx;
    int net_opcode;
    int net_arg1;
    int net_arg2;
	int offset = 0;

	memcpy(&net_primary_id, buffer + offset, sizeof(net_primary_id));
	offset += sizeof(net_primary_id);
	memcpy(&net_last_idx, buffer + offset, sizeof(net_last_idx));
	offset += sizeof(net_last_idx);
	memcpy(&net_committed_idx, buffer + offset, sizeof(net_committed_idx));
	offset += sizeof(net_committed_idx);
	memcpy(&net_opcode, buffer + offset, sizeof(net_opcode));
	offset += sizeof(net_opcode);
	memcpy(&net_arg1, buffer + offset, sizeof(net_arg1));
	offset += sizeof(net_arg1);
	memcpy(&net_arg2, buffer + offset, sizeof(net_arg2));				

	leader_id = ntohl(net_primary_id);
	last_idx = ntohl(net_last_idx);
    committed_idx = ntohl(net_committed_idx); 
    op_code = ntohl(net_opcode);
    op_arg1 = ntohl(net_arg1);
    op_arg2 = ntohl(net_arg2);
}

std::ostream& operator<<(std::ostream& os, const ReplicationRequest& req) {
    os << "**** This is ths replication request ****\n"
	   << "last_idx: " << req.last_idx << ", "
       << "committed_idx: " << req.committed_idx << ", "
       << "leader_id: " << req.leader_id << ", "
       << "op code: " << req.op_code << ", "
	   << "op arg1: " << req.op_arg1 << ", "
	   << "op arg2: " << req.op_arg2 << ", " << std::endl;
    return os;
}


/**
 * Leader info
*/
LeaderInfo::LeaderInfo()
: ip0(0), ip1(0), ip2(0), ip3(0), port(0) {	}

void LeaderInfo::SetLeaderInfo(std::string ip, int port) {
	this->port = port;
	ParseIp(ip);
}

int LeaderInfo::Size() {
	return sizeof(ip0) + sizeof(ip1) + sizeof(ip2) + sizeof(ip3) + sizeof(port);
}

std::string LeaderInfo::GetIp() {
	return std::to_string(ip0) + "." + std::to_string(ip1) + "." + std::to_string(ip2) + "." + std::to_string(ip3);
}

int LeaderInfo::GetPort() {
	return port;
}

void LeaderInfo::Marshal(char *buffer) {
	int net_ip0 = htonl(ip0);
	int net_ip1 = htonl(ip1);
    int net_ip2 = htonl(ip2);
    int net_ip3 = htonl(ip3);
	int net_port = htonl(port);

	int offset = 0;
	memcpy(buffer + offset, &net_ip0, sizeof(net_ip0));
	offset += sizeof(net_ip0);
	memcpy(buffer + offset, &net_ip1, sizeof(net_ip1));
	offset += sizeof(net_ip1);
	memcpy(buffer + offset, &net_ip2, sizeof(net_ip2));
	offset += sizeof(net_ip2);
	memcpy(buffer + offset, &net_ip3, sizeof(net_ip3));
	offset += sizeof(net_ip3);
	memcpy(buffer + offset, &port, sizeof(port));
}

void LeaderInfo::Unmarshal(char *buffer) {
	int net_ip0;
	int net_ip1;
    int net_ip2;
    int net_ip3;
	int net_port;
	int offset = 0;

	memcpy(&net_ip0, buffer + offset, sizeof(net_ip0));
	offset += sizeof(net_ip0);
	memcpy(&net_ip1, buffer + offset, sizeof(net_ip1));
	offset += sizeof(net_ip1);
	memcpy(&net_ip2, buffer + offset, sizeof(net_ip2));
	offset += sizeof(net_ip2);
	memcpy(&net_ip3, buffer + offset, sizeof(net_ip3));
	offset += sizeof(net_ip3);
	memcpy(&net_port, buffer + offset, sizeof(net_port));
	offset += sizeof(net_port);

	ip0 = ntohl(net_ip0);
	ip1 = ntohl(net_ip0);
    ip2 = ntohl(net_ip0); 
    ip3 = ntohl(net_ip0);
    net_port = ntohl(net_port);
}

void LeaderInfo::ParseIp(std::string ip) {
	
	// iterate over the string and add the substring delimited with the . as a single
	int cur_num = 0;
	std::vector<int> ips;
	for (char c : ip) {
		if (c == '.') {
			ips.push_back(cur_num);
			cur_num = 0;
			continue;
		}
		cur_num = cur_num * 10 + (c - '0');
		
	}
	ips.push_back(cur_num);
	ip0 = ips[0];
	ip1 = ips[1];
	ip2 = ips[2];
	ip3 = ips[3];
}