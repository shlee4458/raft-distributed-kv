#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <map>

#include "ServerSocket.h"
#include "ServerThread.h"
#include "ServerMetadata.h"

int main(int argc, char *argv[]) {
	int port, unique_id, num_peers;
	int engineer_cnt = 0;

	ServerSocket socket;
	LaptopFactory factory;
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

	// update the server metadata
	auto metadata = std::make_shared<ServerMetadata>();
	metadata->SetFactoryId(unique_id);
	for (int i = 4, j = 0; j < num_peers; i += 3, j++) {
		// create a node and add the node as the neighbor of the current server
		std::shared_ptr<ServerNode> node = std::shared_ptr<ServerNode>(new ServerNode());
		node->id = atoi(argv[i]);
		node->ip = argv[i + 1];
		node->port = atoi(argv[i + 2]);
		std::cout << "Created peer node: " << j + 1 << std::endl;

		metadata->AddNeighbors(std::move(node));
	}

	// create the primary admin thread
	std::thread pfa_thread(&LaptopFactory::PrimaryAdminThread, 
			&factory, engineer_cnt++);
	thread_vector.push_back(std::move(pfa_thread));
	
	// create the idle admin thread
	std::thread ifa_thread(&LaptopFactory::IdleAdminThread,
			&factory, engineer_cnt++);
	thread_vector.push_back(std::move(ifa_thread));

	if (!socket.Init(port)) {
		std::cout << "Socket initialization failed" << std::endl;
		return 0;
	}

	while ((new_socket = socket.Accept())) {
		std::cout << "I have received the connection request from the primary" << std::endl;
		std::thread engineer_thread(&LaptopFactory::EngineerThread, &factory, 
				std::move(new_socket), engineer_cnt++, metadata);
		thread_vector.push_back(std::move(engineer_thread));
	}
	return 0;
}
