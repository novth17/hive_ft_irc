#pragma once

#include <iostream>
#include <vector>
#include <map>
#include "Connection.hpp"

// class Server {
// 	private:
// 		int _serverFd;
// 		int _epollFd;
// 		std::map<int, Connection> _connections;

// 	public:
// 		Server() = delete;
// 		Server(const std::string&);
// 		Server(const Server& Server) = delete;
// 		Server &operator=(const Server& Server) = delete;
// 		~Server();

// 		void handleNewConnection();
// 		void handleClientRead(int fd); //epollin
// 		void handleClientWrite(int fd); //epollout

// 		void updateEpoll(int fd, uint32_t events);

// 		int eventLoop(const char* host, const char* port);
// 		std::vector<Server> servers;
// 		std::vector<Connection> connections;

// };
