#pragma once

#include <set>
#include <string>
#include <string_view>

class Client;
class Server;

class Channel
{
public:
	explicit Channel(std::string_view name);
	~Channel() = default;

	std::set<Client*> members;		// All clients joined to this channel
	std::set<Client*> operators;	// All clients with operator privileges
	std::string topicChangeStr;		// The nick of the person who last changed topic plus a timestamp
	bool inviteOnly = false;		// Whether the +i mode is set
	bool restrictTopic = false;		// Whether the +t mode is set
	int memberLimit = 0;			// Limit for the +l mode (0 = no limit)

	bool isMember(Client& client) const;
	void addMember(Client& client);
	void removeMember(Client& client);

	bool isOperator(Client& client) const;
	void addOperator(Client& client);
	void removeOperator(Client& client);

	bool hasKey() const;
	std::string_view getKey() const;
	bool setKey(std::string_view newKey);
	void removeKey();

	void setMemberLimit(int limit);
	void removeMemberLimit();
	bool isFull() const;
	bool isEmpty() const;
	int getMemberCount() const;

	bool isInvited(Client& invited) const;
	void addInvited(Client& invited);
	void removeInvited(Client& invited);
	void resetInvited();

	std::string getModes() const;
	static bool isValidName(std::string_view name);
	Client* findClientByName(std::string_view name);

	std::string_view getName() const;

	bool hasTopic() const;
	std::string_view getTopic() const;
	void setTopic(std::string_view newTopic, Client& client);

private:
	std::string name;				// The name of the channel
	std::string topic;				// The current topic
	std::string key;				// Key for the +k mode (empty = no key)
	std::set<Client*> invited;		// All nicknames invited to this channel
};
