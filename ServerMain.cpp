#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <map>
#include <chrono>

#include "ServerSocket.h"
#include "ServerThread.h"
#include "ServerMetadata.h"

int main(int argc, char *argv[]) {
	int port, unique_id, num_peers;
	int engineer_cnt = 0;

	ServerSocket socket;
	std::shared_ptr<ServerSocket> new_socket;
	std::vector<std::thread> thread_vector;

	if (argc < 4 || (argc - 4) % 3 != 0) {
		std::cout << "not enough arguments or does not have enough information for neighboring nodes" << std::endl;
		std::cout << argv[0] << "[port #] [unique ID] [# peers]" << std::endl;
		return 0;
	}

	port = atoi(argv[1]);
	unique_id = atoi(argv[2]);
	num_peers = atoi(argv[3]);
	std::cout << "num_peers: " << num_peers << std::endl;

	if ((argc - 4) / 3 != num_peers) {
		std::cout << "not enough peer information has been provided!" << std::endl;
		return 0;
	}

	if (!socket.Init(port)) {
		std::cout << "Socket initialization failed" << std::endl;
		return 0;
	}

	// update the server metadatas
	auto metadata = std::make_shared<ServerMetadata>();
	metadata->SetFactoryId(unique_id);
	for (int i = 4, j = 0; j < num_peers; i += 3, j++) {
		// create a node and add the node as the neighbor of the current server
		auto node = std::make_shared<ServerNode>();
		node->id = atoi(argv[i]);
		node->ip = argv[i + 1];
		node->port = atoi(argv[i + 2]);
		std::cout << "Created peer node: " << j + 1 << std::endl;
		metadata->AddNeighbors(std::move(node), j);
	}
	// give 5 seconds to allow all servers to boot, and connect with the neighbors
	std::this_thread::sleep_for(std::chrono::seconds(3));

	metadata->InitNeighbors(); 
	LaptopFactory factory(std::move(metadata));
	
	// create the leader thread
	std::thread leader_thread(&LaptopFactory::LeaderThread, 
			&factory, engineer_cnt++);
	thread_vector.push_back(std::move(leader_thread));

	// create the follower admin thread
	std::thread follower_thread(&LaptopFactory::FollowerThread,
			&factory);
	thread_vector.push_back(std::move(follower_thread));	
	
	// create the candidate thread
		// this will initialize
	std::thread timeout_thread(&LaptopFactory::TimeoutThread,
			&factory);
	thread_vector.push_back(std::move(timeout_thread));

	// TODO: consider changing to have two different sockets for appendRPC & voteRPC
		// use two separate threads to 
	while ((new_socket = socket.Accept())) {
		std::cout << "I have received the connection request!" << std::endl;
		std::thread engineer_thread(&LaptopFactory::EngineerThread, &factory, 
				std::move(new_socket), engineer_cnt++);
		thread_vector.push_back(std::move(engineer_thread));
	}
	
	return 0;
}
