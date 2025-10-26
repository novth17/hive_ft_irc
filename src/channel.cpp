#include "channel.hpp"
#include "client.hpp"
#include "utility.hpp"

/**
 * Make a new channel.
 */
Channel::Channel(std::string_view name)
	: name(name)
{
}

/**
 * Check if a client is a member of this channel.
 */
bool Channel::isMember(Client& client) const
{
	return members.find(&client) != members.end();
}

/**
 * Make a client a member of this channel. If it's the first member of the
 * channel, then it's also made the channel operator.
 */
void Channel::addMember(Client& client)
{
	members.insert(&client);
	if (members.size() == 1) {
		addOperator(client);
		client.sendLine("MODE ", name, " +o ", client.nick);
	}
}

/**
 * Remove a client from a channel. The client is also removed from the operator
 * and invite lists, if applicable.
 */
void Channel::removeMember(Client& client)
{
	members.erase(&client);
	operators.erase(&client);
	invited.erase(&client);
}

/**
 * Check if a client is an operator for this channel.
 */
bool Channel::isOperator(Client& client) const
{
	return operators.find(&client) != operators.end();
}

/**
 * Give a client channel operator privileges. Also sends a MODE message to the
 * channel, letting everyone know that they're now an operator.
 */
void Channel::addOperator(Client& client)
{
	operators.insert(&client);
}

/**
 * Remove channel operator privileges from a client.
 */
void Channel::removeOperator(Client& client)
{
	operators.erase(&client);
}

/**
 * Determine if a string contains a valid channel name. A channel must start
 * with either a '#' or '&' sign, and can't contain spaces, commas or the bell
 * character (of all things).
 */
bool Channel::isValidName(std::string_view name)
{
	if (name.empty() || (name[0] != '#' && name[0] != '&'))
		return false;
	for (char chr: name)
		if (chr == ' ' || chr == ',' || chr == '\a')
			return false;
	return true;
}

/**
 * Find a client by nickname among the members of the channel. Returns a null
 * pointer if no client by that name was found.
 */
Client* Channel::findClientByName(std::string_view nick)
{
	for (Client* client: members)
		if (client->nick == nick)
			return client;
	return nullptr;
}

/**
 * Get a string representing the current modes set for the channel, including
 * the mode arguments. The key is not included. ("Servers MAY choose to hide
 * sensitive information such as channel keys when sending the current modes").
 */
std::string Channel::getModes() const
{
	std::string modes;
	if (inviteOnly)
		modes += "i";
	if (restrictTopic)
		modes += "t";
	if (!key.empty())
		modes += "k";
	if (memberLimit > 0)
		modes += "l " + std::to_string(memberLimit);
	return modes.empty() ? "" : "+" + modes;
}

/**
 * Check if a channel has a key (channels don't by default).
 */
bool Channel::hasKey() const
{
	return !key.empty();
}

/**
 * Get the channel key.
 */
std::string_view Channel::getKey() const
{
	return key;
}

/**
 * Set the channel key. Returns true if the new key was set, or false if the key
 * is empty or contains invalid characters.
 */
bool Channel::setKey(std::string_view newKey)
{
	if (newKey.empty())
		return false;
	for (char c: newKey)
		if (std::isspace(c))
			return false;
	key = newKey;
	return true;
}

/**
 * Remove the channel key requirement.
 */
void Channel::removeKey()
{
	key.clear();
}

/**
 * Set the channel member limit. The limit must be higher than zero.
 */
void Channel::setMemberLimit(int limit)
{
	assert(limit > 0);
	memberLimit = limit;
}

/**
 * Remove the channel member limit.
 */
void Channel::removeMemberLimit()
{
	memberLimit = 0;
}

/**
 * Check if a client is on the invite list.
 */
bool Channel::isInvited(Client& client) const
{
	return invited.contains(&client);
}

/**
 * Add a client to the invite list.
 */
void Channel::addInvited(Client& client)
{
	invited.insert(&client);
}

/**
 * Remove a client from the invite list.
 */
void Channel::removeInvited(Client& client)
{
	invited.erase(&client);
}

/**
 * Reset the invite list.
 */
void Channel::resetInvited()
{
	invited.clear();
}

/**
 * Check if the channel is full, i.e. it has a member limit, and that limit has
 * been reached.
 */
bool Channel::isFull() const
{
	return memberLimit != 0 && static_cast<int>(members.size()) >= memberLimit;
}

/**
 * Check if the channel is empty, meaning there are no clients joined to the
 * channel.
 */
bool Channel::isEmpty() const
{
	return members.empty();
}

/**
 * Get the number of clients joined to the channel.
 */
int Channel::getMemberCount() const
{
	return static_cast<int>(members.size());
}

/**
 * Get the name of the channel, including the '#' prefix.
 */
std::string_view Channel::getName() const
{
	return name;
}

/**
 * Check if the channel currently has a topic.
 */
bool Channel::hasTopic() const
{
	return !topic.empty();
}

/**
 * Get the current topic (or an empty string if there's none).
 */
std::string_view Channel::getTopic() const
{
	return topic;
}

/**
 * Set the current topic. Also updates the topic change nickname and timestamp.
 */
void Channel::setTopic(std::string_view newTopic, Client& client)
{
	topic = newTopic;
	topicChangeStr = client.nick + " " + Server::getTimeString();
	log::info(client.nick, " changed topic of ", name, " to: ", newTopic);
}
