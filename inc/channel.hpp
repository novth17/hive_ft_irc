#pragma once

#include <climits>
#include <set>
#include <string>
#include <string_view>

class Client;
class Server;

struct MemberIterators
{
	std::set<Client*>::iterator first, last;
	auto begin() { return first; }
	auto end() { return last; }
};

class Channel
{
public:
	explicit Channel(std::string_view name);
	~Channel() = default;

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

	MemberIterators allMembers();
	int getMemberLimit() const;
	void setMemberLimit(int limit);
	bool isFull() const;
	bool isEmpty() const;
	int getMemberCount() const;

	bool isInviteOnly() const;
	void setInviteOnly(bool enable);

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
	std::string_view getTopicChange() const;
	void setTopic(std::string_view newTopic, Client& client);

	bool isTopicRestricted() const;
	void setTopicRestricted(bool enable);

private:
	std::string name;				// The name of the channel
	std::string topic;				// The current topic
	std::string topicChangeStr;		// The nick of the person who last changed topic plus a timestamp
	std::string key;				// Key for the +k mode (empty = no key)
	std::set<Client*> members;		// All clients joined to this channel
	std::set<Client*> operators;	// All clients with operator privileges
	std::set<Client*> invited;		// All nicknames invited to this channel
	bool inviteOnly = false;		// Whether the +i mode is set
	bool topicRestricted = false;	// Whether the +t mode is set
	int memberLimit = INT_MAX;		// Limit for the +l mode
};
