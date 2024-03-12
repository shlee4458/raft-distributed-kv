#include "ServerMetadata.h"
#include "Messages.h"

#include <string.h>
#include <iostream>

#define PFA_IDENTIFIER 1
#define DEBUG 0

ServerMetadata::ServerMetadata() 
: last_idx(-1), committed_idx(-1), leader_id(-1), factory_id(-1), neighbors() { }

int ServerMetadata::GetLeaderId() {
    return leader_id;
}

int ServerMetadata::GetFactoryId() {
    return factory_id;
}

int ServerMetadata::GetCommittedIndex() {
    return committed_idx;
}

int ServerMetadata::GetLastIndex() {
    return last_idx;
}

std::vector<std::shared_ptr<ServerNode>> ServerMetadata::GetNeighbors() {
    return neighbors;
}

int ServerMetadata::GetPeerSize() {
    return neighbors.size();
}

std::deque<std::shared_ptr<ServerNode>> ServerMetadata::GetFailedNeighbors() {
    return failed_neighbors;
}

std::deque<std::shared_ptr<ClientSocket>> ServerMetadata::GetPrimarySockets() {
    return primary_sockets;
}

std::vector<MapOp> ServerMetadata::GetLog() {
    return smr_log;
}

MapOp ServerMetadata::GetOp(int idx) {
    return smr_log[idx];
}

int ServerMetadata::GetValue(int customer_id) {
    auto it = customer_record.find(customer_id);
    if (it != customer_record.end()) { // found the key, return the value
        return customer_record[customer_id];
    } else { // key not found, return -1
        std::cout << "Key not found!" << std::endl;
        return -1; 
    }
}

ReplicationRequest ServerMetadata::GetReplicationRequest(MapOp op) {
    int op_code = op.opcode;
    int op_arg1 = op.arg1;
    int op_arg2 = op.arg2;
    return ReplicationRequest(last_idx, committed_idx, leader_id, op_code, op_arg1, op_arg2);
}

void ServerMetadata::SetLeaderId(int id) {
	leader_id = id;
    return;
}

void ServerMetadata::SetFactoryId(int id) {
    factory_id = id;
    return;
}

void ServerMetadata::UpdateLastIndex(int idx) {
    last_idx = idx;
    return;
}

void ServerMetadata::UpdateCommitedIndex(int idx) {
    committed_idx = idx;
    return;
}

void ServerMetadata::AppendLog(MapOp op) {
    smr_log.push_back(op);
    last_idx++;
    return;
}

void ServerMetadata::ExecuteLog(int idx) {
    int customer_id, order_num;
    
    MapOp op = GetOp(idx);
    customer_id = op.arg1;
    order_num = op.arg2;

    customer_record[customer_id] = order_num;
    if (DEBUG) {
        std::cout << "Record Updated for client: " << customer_id 
            << " Order Num: " << order_num << std::endl;
    } 
    committed_idx++;
    return;
}

bool ServerMetadata::WasBackup() {
    return leader_id != -1;
}

bool ServerMetadata::IsLeader() {
    return leader_id == factory_id;
}

void ServerMetadata::AddNeighbors(std::shared_ptr<ServerNode> node) {
    neighbors.push_back(std::move(node));
}

void ServerMetadata::InitNeighbors() {

    // corner case: primary -> idle -> primary; empty the sockets and failed
    primary_sockets.clear();
    failed_neighbors.clear();

	std::string ip;
	int port;

	for (const auto& node : GetNeighbors()) {
		port = node->port;
		ip = node->ip;
		std::shared_ptr<ClientSocket> socket = std::make_shared<ClientSocket>();
		if (socket->Init(ip, port)) { // if connection is successful
            socket_node[socket] = node;
            SendIdentifier(socket);
            primary_sockets.push_back(std::move(socket));
        } else { // if there is a failed server, add to the deque
            failed_neighbors.push_back(node);
        }
	}
}

int ServerMetadata::SendIdentifier(std::shared_ptr<ClientSocket> socket) {

	// send identifier to the idle server
	char identifier_buffer[4];
	int identifier_size;
	auto identifier = std::shared_ptr<Identifier>(new Identifier());

	identifier->SetIdentifier(PFA_IDENTIFIER);
	identifier->Marshal(identifier_buffer); // store the identifier value in the buffer
	identifier_size = identifier->Size();

    if (DEBUG) {
        std::cout << "There is a peer" << std::endl;
    }
    if (!socket->Send(identifier_buffer, identifier_size)) {
        return 0; // failed to send an identifier to an idle server
    }
	return 1;
}

int ServerMetadata::SendReplicationRequest(MapOp op) {

	char buffer[32];
    int size;

    // get replication request object
	ReplicationRequest request = GetReplicationRequest(op);
	request.Marshal(buffer);
	size = request.Size();

	int total_response = failed_neighbors.size();
	char response_buffer[4];
	Identifier identifier;
    std::deque<std::shared_ptr<ClientSocket>> new_primary_sockets;

    // iterate over all the neighbor nodes, and send the replication request
	for (auto const& socket : primary_sockets) {

		// if any one of the idle servers failed
			// consider response was received
			// continue sending it to the other idle servers
            // update the failed_neighbor to include the current failed server node
		if (!socket->Send(buffer, size, 0)) {
            failed_neighbors.push_back(socket_node[socket]);
			total_response++;
			continue;
		}

		if (socket->Recv(response_buffer, sizeof(identifier), 0)) {
			identifier.Unmarshal(response_buffer);
			total_response += identifier.GetIdentifier();
		}
        new_primary_sockets.push_back(socket);
	}

    // update with the sockets excluding failed sockets
    primary_sockets = new_primary_sockets; 
    if (DEBUG) {
        std::cout << request << std::endl;
    }
    
    // check if the message received matches the size of the neighbors
	if (total_response != GetPeerSize()) {
        if (DEBUG) {
            // std::cout << GetPeerSize() << " " << total_response << std::endl;
            std::cout << "Some neighbor has not updated the log, so I am not executing the log!" << std::endl;
        }
		return 0;
	}

    return 1;
}

std::shared_ptr<ServerNode> ServerMetadata::GetLeader() {
    for (std::shared_ptr<ServerNode> nei: GetNeighbors()) {
        if (nei->id == GetLeaderId()) {
            return nei;
        }
    }
}

std::string ServerMetadata::GetLeaderIp() {
    std::shared_ptr<ServerNode> leader = GetLeader();
    return leader->ip;
}

int ServerMetadata::GetLeaderPort() {
    std::shared_ptr<ServerNode> leader = GetLeader();
    return leader->port;
}

int ServerMetadata::GetStatus() {
    return status;
}

void ServerMetadata::SetStatus(int status) {
    this->status = status;
}