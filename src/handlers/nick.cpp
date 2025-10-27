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
	return !name.empty()
		&& name.length() <= NICKLEN
		&& !name.starts_with(':')
		&& !name.starts_with('#')
		&& name.find(' ') == name.npos;
}

/**
 * Handle a NICK message.
 */
void Client::handleNick(int argc, char** argv)
{
	if (!checkParams("NICK", false, argc, 1, 1))
		return;

	// Must have passed the correct password first: https://datatracker.ietf.org/doc/html/rfc2812#section-3.1.1
	if (!isPassValid) {
		log::warn(user, " NICK: password is not yet set");
		return sendNumeric("464", ":Password incorrect");
	}

	// Check that the new nick is valid and not in use.
	std::string_view newNick = argv[0];
	if (server.findClientByName(newNick)) {
		log::warn(user, " NICK: Nickname is already in use: ", newNick);
		return sendNumeric("433", newNick, " :Nickname is already in use");
	}
	if (!isValidName(newNick)) {
		log::warn(user, " NICK: Invalid format of nickname");
		return sendNumeric("432", newNick, " :Erroneus nickname");
	}

	// Send a notification of the name change to the client, and to other
	// channel members.
	if (isRegistered) {
		sendLine(":", fullname, " NICK ", newNick);
		for (Channel* channel: channels)
			for (Client* member: channel->allMembers())
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
