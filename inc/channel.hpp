#pragma once

#include <set>
#include <string>
#include <string_view>

class Client;
class Server;

class Channel
{
public:

	char symbol = '=';			// The channel symbol (one of '=', '@', '*')
	std::string name;			// The name of the channel
	std::string topic;			// The current topic
	std::string key;			// Key needed to join the channel
	std::string modes;			// Channel mode string
	std::set<Client*> members;	// All clients joined to this channel
	Server* server = nullptr;	// Pointer to the server object.

	static bool isValidName(std::string_view name);
	Client* findClientByName(std::string_view name);
};
