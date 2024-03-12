#ifndef __SERVERMETADATA_H__
#define __SERVERMETADATA_H__

#include <vector>
#include <memory>
#include <map>
#include <mutex>
#include <string.h>
#include <deque>

#include "ClientSocket.h"
#include "Messages.h"

struct ServerNode {
    int id;
    std::string ip;
    int port;
};

struct MapOp {
	int opcode; // operation code : 1 - update value
	int arg1; // customer_id to apply the operation
	int arg2; // parameter for the operation
};

class ServerMetadata {
private:
    int last_idx;
    int committed_idx;
    int primary_id;
    int factory_id;
    bool is_primary = false;
    std::vector<std::shared_ptr<ServerNode>> neighbors;
    std::deque<std::shared_ptr<ClientSocket>> primary_sockets; // socket to the backup nodes as a primary
    std::map<int, int> customer_record;
    std::vector<MapOp> smr_log;
    std::deque<std::shared_ptr<ServerNode>> failed_neighbors; // store the ServerNodes that are not open
    std::map<std::shared_ptr<ClientSocket>, std::shared_ptr<ServerNode>> socket_node;

public:
    ServerMetadata();

    int GetPrimaryId();
    int GetFactoryId();
    int GetLastIndex();
    int GetCommittedIndex();
    int GetPeerSize();
    std::vector<MapOp> GetLog();
    MapOp GetOp(int idx);
    std::vector<std::shared_ptr<ServerNode>> GetNeighbors();
    std::deque<std::shared_ptr<ClientSocket>> GetPrimarySockets();
    std::deque<std::shared_ptr<ServerNode>> GetFailedNeighbors();
    int GetValue(int customer_id);
    ReplicationRequest GetReplicationRequest(MapOp op);

    void SetFactoryId(int id);
    void SetPrimaryId(int id);
    void UpdateLastIndex(int idx);
    void UpdateCommitedIndex(int idx);
    void AppendLog(MapOp op);
    void ExecuteLog(int idx);
    

    bool WasBackup();
    bool IsPrimary();

    void AddNeighbors(std::shared_ptr<ServerNode> node);
    void InitNeighbors();
    void RepairFailedServers();
    int SendReplicationRequest(MapOp op);
    int Repair(std::shared_ptr<ClientSocket> socket);
    int SendIdentifier(std::shared_ptr<ClientSocket> socket);
};

#endif