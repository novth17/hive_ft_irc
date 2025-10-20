#pragma once

#include <map>
#include <string>

class Channel;
class Client;

class Server
{
public:
	Server(const char* port, const char* password);
	~Server();

	Channel* findChannelByName(std::string_view name);
	Client* findClientByName(std::string_view name);
	Channel* newChannel(const std::string& name);
	void eventLoop(const char* host, const char* port);

private:
	bool setNonBlocking(int fd);
	void closeAndClean(const std::string& msg, int sockfd, struct addrinfo* result);
	int  createListenSocket(const char* host, const char* port, bool isNonBlocking);

	void receiveFromClient(Client& client);

	void parseMessage(Client& client, std::string message);
	void handleMessage(Client& client, int argc, char** argv);

	// Handlers for specific messages.
	void handleUser(Client& client, int argc, char** argv);
	void handleNick(Client& client, int argc, char** argv);
	void handlePass(Client& client, int argc, char** argv);
	void handleCap(Client& client, int argc, char** argv);
	void handleJoin(Client& client, int argc, char** argv);

	int serverFd = -1;
	int epollFd = -1;
	std::string port;
	std::string password;
	std::map<int, Client> clients;
	std::map<std::string, Channel> channels;
};
