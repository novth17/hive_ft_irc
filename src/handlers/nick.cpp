#include <cstring>

#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "server.hpp"
#include "irc.hpp"

/**
 * Check if a string contains a valid nickname.
 */
bool Client::isValidName(std::string_view name)
{
	return !name.starts_with(':')
		&& !name.starts_with('#')
		&& name.find(' ') == name.npos;
}

/**
 * Handle a NICK message.
 */
void Client::handleNick(int argc, char** argv)
{
	// Must have passed the correct password first: https://datatracker.ietf.org/doc/html/rfc2812#section-3.1.1
	if (!isPassValid) {
		sendLine("464 :Password incorrect");
		return log::warn(user, " Password is not yet set");
	}
	// Check the parameter count.
	if (argc < 1) {
		sendLine("431 ", nick, " :No nickname given");
		return log::warn(user, " NICK: No nick name given yet");
	}
	std::string_view newNick = argv[0];
	if (argc > 2) {
		sendLine("432 ", nick, " ", newNick, " :Erroneus nickname");
		return log::warn(user, " NICK: Too many parameters given");
	}
	// Check that the new nick is valid and not in use.
	if (server->findClientByName(newNick)) {
		sendLine("433 ", nick, " ", newNick, " :Nickname is already in use");
		return log::warn(user, " NICK: Nickname is same as current one. Current Nickname: ", nick);
	}
	if (!isValidName(newNick)) {
		sendLine("432 ", nick, " ", newNick, " :Erroneus nickname");
		return log::warn(user, " NICK: Invalid format of nickname");
	}

	// Send a notification of the name change to the client, and to other
	// channel members.
	if (isRegistered) {
		sendLine(":", fullname, " NICK ", newNick);
		for (Channel* channel: channels)
			for (Client* member: channel->members)
				if (member != this)
					member->sendLine(":", fullname, " NICK ", newNick);
	}

	// Update the nick, and complete registration, if applicable.
	bool nickAlreadySubmitted = !nick.empty();
	nick = newNick;
	fullname = nick + "!" + user + "@" + host;
	if (!nickAlreadySubmitted)
		handleRegistrationComplete();
}
