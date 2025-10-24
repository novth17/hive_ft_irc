#pragma once

#include <set>
#include <string>
#include <string_view>

class Client;
class Server;

class Channel
{
public:

	char symbol = '=';				// The channel symbol (one of '=', '@', '*')
	std::string name;				// The name of the channel
	std::string topic;				// The current topic
	std::set<Client*> members;		// All clients joined to this channel
	std::set<Client*> operators;	// All clients with operator privileges
	Server* server = nullptr;		// Pointer to the server object.
	std::string topicChangeStr;		// The nick of the person who last changed topic plus a timestamp
	bool inviteOnly = false;		// Whether the +i mode is set
	bool restrictTopic = false;		// Whether the +t mode is set
	std::string key;				// Key for the +k mode (empty = no key)
	int memberLimit = 0;			// Limit for the +l mode (0 = no limit)

	bool isMember(Client& client) const;
	void addMember(Client& client);
	void removeMember(Client& client);

	bool isOperator(Client& client) const;
	void addOperator(Client& client);
	void removeOperator(Client& client);

	bool setKey(std::string_view newKey);
	void removeKey();

	void setMemberLimit(int limit);
	void removeMemberLimit();
	bool isFull() const;

	bool isInvited(std::string_view invited);
	void addInvited(std::string_view invited);
	void resetInvited();

	std::string getModes() const;
	static bool isValidName(std::string_view name);
	Client* findClientByName(std::string_view name);

private:
	std::set<std::string> invited;	// All nicknames invited to this channel

};
