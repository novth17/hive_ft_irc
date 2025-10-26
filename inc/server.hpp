#pragma once

#include <map>
#include <string>
#include <string_view>

class Channel;
class Client;

struct ClientIterators
{
	std::map<int, Client>::iterator first, last;
	auto begin() { return first; }
	auto end() { return last; }
};

struct ChannelIterators
{
	std::map<std::string, Channel>::iterator first, last;
	auto begin() { return first; }
	auto end() { return last; }
};

class Server
{
public:
	Server(const char* port, const char* password);
	~Server();

	Channel* findChannelByName(std::string_view name);
	Client* findClientByName(std::string_view name);
	Channel* newChannel(const std::string& name);
	void eventLoop(const char* host, const char* port);
	bool correctPassword(std::string_view pass);
	bool clientsOnSameChannel(const Client& a, const Client& b);
	void disconnectClient(Client& client, std::string_view reason = "");
	std::string getLaunchTime();
	static std::string getTimeString();
	size_t getClientCount() const;
	size_t getChannelCount() const;
	ClientIterators allClients() { return {clients.begin(), clients.end()}; }
	ChannelIterators allChannels() { return {channels.begin(), channels.end()}; }
	std::string_view getHostname();

private:
	void closeAndClean(const std::string& msg, int sockfd, struct addrinfo* result);
	int  createListenSocket(const char* host, const char* port);
	void receiveFromClient(Client& client);
	void parseMessage(Client& client, std::string message);
	void handleMessage(Client& client, int argc, char** argv);

	int serverFd = -1;
	int epollFd = -1;
	std::string launchTime;
	std::string port;
	std::string password;
	std::string hostname;
	std::map<int, Client> clients;
	std::map<std::string, Channel> channels;
};
