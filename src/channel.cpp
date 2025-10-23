#include "channel.hpp"
#include "client.hpp"
#include "log.hpp"

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
	if (members.empty())
		operators.insert(&client);
	members.insert(&client);
}

/**
 * Remove a client from a channel. If the client is also a channel operator,
 * it's also removed from the channel operator list.
 */
void Channel::removeMember(Client& client)
{
	members.erase(&client);
	operators.erase(&client);
}

/**
 * Check if a client is an operator for this channel.
 */
bool Channel::isOperator(Client& client) const
{
	return operators.find(&client) != operators.end();
}

/**
 * Give a client channel operator privileges.
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
	if (clientLimit > 0)
		modes += "l " + std::to_string(clientLimit);
	return modes.empty() ? "" : "+" + modes;
}
