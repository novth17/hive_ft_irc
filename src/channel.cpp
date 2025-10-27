#include <ctime>

#include "channel.hpp"
#include "client.hpp"
#include "irc.hpp"
#include "utility.hpp"

/**
 * Make a new channel.
 */
Channel::Channel(std::string_view name)
	: name(name),
	  creationTime(time(nullptr))
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
 * Make a client a member of this channel.
 */
void Channel::addMember(Client& client)
{
	members.insert(&client);
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
 * with either a '#', and can't contain spaces, commas or the bell
 * character (of all things).
 */
bool Channel::isValidName(std::string_view name)
{
	if (name.empty() || name.length() > CHANNELLEN
		|| name[0] != '#')
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
		if (client->getNick() == nick)
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
	if (topicRestricted)
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
 * Get an iterator pair over all channel members.
 */
MemberIterators Channel::allMembers()
{
	return {members.begin(), members.end()};
}

/**
 * Get the channel member limit.
 */
int Channel::getMemberLimit() const
{
	return memberLimit;
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
 * Check if the channel is invite-only (mode +i).
 */
bool Channel::isInviteOnly() const
{
	return inviteOnly;
}

/**
 * Set whether a channel is invite-only.
 */
void Channel::setInviteOnly(bool enable)
{
	inviteOnly = enable;
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
	return static_cast<int>(members.size()) >= memberLimit;
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
 * Get the nick and timestamp of the last topic change.
 */
std::string_view Channel::getTopicChange() const
{
	return topicChangeStr;
}

/**
 * Set the current topic. Also updates the topic change nickname and timestamp.
 */
void Channel::setTopic(std::string_view newTopic, Client& client)
{
	topic = newTopic;
	topicChangeStr = std::string(client.getNick()) + " " + Server::getTimeString();
	log::info(client.getNick(), " changed topic of ", name, " to: ", newTopic);
}

/**
 * Get whether or not the topic is restricted (meaning it can only be changed by
 * a channel operator).
 */
bool Channel::isTopicRestricted() const
{
	return topicRestricted;
}

/**
 * Set whether or not the topic is restricted.
 */
void Channel::setTopicRestricted(bool enable)
{
	topicRestricted = enable;
}

/**
 * Get the Unix timestamp for the channel creation time.
 */
int64_t Channel::getCreationTime() const
{
	return creationTime;
}
