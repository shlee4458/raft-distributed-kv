#ifndef __SERVERMETADATA_H__
#define __SERVERMETADATA_H__

#include <vector>
#include <memory>
#include <map>
#include <mutex>
#include <string.h>
#include <deque>
#include <set>

#include "ClientSocket.h"
#include "Messages.h"

struct ServerNode {
    int id;
    std::string ip;
    int port;
};

struct MapOp {
	int term; // term of the op
	int arg1; // customer_id to apply the operation
	int arg2; // parameter for the operation
};

class ServerMetadata {
private:
    int last_idx;
    int committed_idx;
    int leader_id; // -1 
    int factory_id;
    bool is_leader = false;
    int status = 1;
    int commit_length;

    int voted_for;
    std::set<int> vote_received;
    int current_term;
    int* sent_length; // reserve sent_length[size] for itself
    int* ack_length; // reserve ack_length[size] for itself

    std::vector<std::shared_ptr<ServerNode>> neighbors;
    std::deque<std::shared_ptr<ClientSocket>> neighbor_sockets; // socket to the backup nodes as a primary
    std::map<int, int> customer_record;
    std::vector<MapOp> smr_log;
    std::map<std::shared_ptr<ClientSocket>, std::shared_ptr<ServerNode>> socket_node;

public:
    ServerMetadata();

    int GetLeaderId();
    int GetFactoryId();
    int GetLastIndex();
    int GetCommittedIndex();
    int GetPeerSize();
    std::vector<MapOp> GetLog();
    MapOp GetOp(int idx);
    std::vector<std::shared_ptr<ServerNode>> GetNeighbors();
    std::deque<std::shared_ptr<ClientSocket>> GetNeighborSockets();
    int GetValue(int customer_id);
    ReplicationRequest GetReplicationRequest(MapOp op);
    std::shared_ptr<ServerNode> GetLeader();
    std::string GetLeaderIp();
    int GetLeaderPort();
    int GetStatus();
    int GetVoteReceivedSize();
    int GetCurrentTerm();

    void SetFactoryId(int id);
    void SetLeaderId(int id);
    void UpdateLastIndex(int idx);
    void UpdateCommitedIndex(int idx);
    void SetStatus(int status);
    void SetCurrentTerm(int term);
    void SetVotedFor(int id);

    void AppendLog(MapOp op);
    void ExecuteLog(int idx);

    bool IsLeader();
    bool WonElection();

    void AddNeighbors(std::shared_ptr<ServerNode> node);
    void InitNeighbors();
    int SendReplicationRequest(MapOp op);

    void ReplicateLog();
    void RequestVote();
    int SendIdentifier(int identifier, std::shared_ptr<ClientSocket> nei);
    int GetLogSize();
    int GetLastTerm();
    bool GetVotedFor();
    RequestVoteResponse RecvVoteResponse(std::shared_ptr<ClientSocket> nei);
    void InitLeader();
    void SetAckLength(int node_idx, int size);
    void SendLog(LogRequest lr, int node_idx);
};

#endif