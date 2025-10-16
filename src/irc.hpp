#pragma once

#include <iostream>
#include <map>


#define PORT_MAX 65535 // The maximum allowed port number.
#define MAX_BACKLOG 20 // Maximum length of the pending connection queue.

class Server
{
public:
    Server(const char* port, const char* password);
    ~Server();

    //void start();
	void handleNewConnection();
	int eventLoop(const char* host, const char* port);

	private:
    int _sockfd = -1;

	const char* _port = nullptr;
    const char* _password = nullptr;

	int _serverFd;
	int _epollFd;
	//std::map<int, Connection> _connections;
};

class Client
{
public:
    Client(const std::string& nick);

private:
    std::string nick;
    bool isOperator = false;
};

class Channel
{
public:
    Channel(const std::string& name);

private:
    std::string name;
    std::string topic;
};
