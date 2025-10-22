#include "channel.hpp"
#include "client.hpp"
#include "log.hpp"

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
