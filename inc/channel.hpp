#pragma once

#include <map>
#include <string>

class Client;
class Server;

class Channel
{
public:

	char symbol = '=';						// The channel symbol (one of '=', '@', '*')
	std::string name;						// The name of the channel
	std::string topic;						// The current topic
	std::string key;						// Key needed to join the channel
	std::map<std::string, Client*> members;	// Clients joined to this channel
	Server* server = nullptr;				// Pointer to the server object.

	static bool isValidName(const char* name);
};
