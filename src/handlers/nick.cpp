#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "server.hpp"
#include "irc.hpp"
#include <cstring>

/**
 * Handle a NICK message.
 */
void Client::handleNick(int argc, char** argv)
{
	if (!isPassValid) { // Must have passed the correct password first: https://datatracker.ietf.org/doc/html/rfc2812#section-3.1.1
		sendLine("464 :Password incorrect");
		return log::warn(user, "NICK: Password is not yet set");
	}

	if (argc == 0) {
		sendLine("431 ", nick, " :No nickname given");
		return log::warn(user, "NICK: No nick name given yet");
	}

	std::string_view newNick = argv[0];
	if (argc > 2){
		sendLine("432 ", nick, " ", newNick, " :Erroneus nickname");
		return log::warn(user, "NICK: Too many parameters given");

	}
	bool nickAlreadySubmitted = !nick.empty();

	if (server->findClientByName(newNick)) {
		sendLine("433 ", nick, " ", newNick, " :Nickname is already in use");
		return log::warn(user, "NICK: Nickname is same as current one. Current Nickname: ", nick);
	}
	if ((newNick[0] == ':') || (newNick[0] == '#')
		|| std::string_view(newNick).find(' ') != std::string::npos) {
		sendLine("432 ", nick, " ", newNick, " :Erroneus nickname");
		return log::warn(user, "NICK: Invalid format of nickname");
	}
	if (isRegistered)
		for (Channel* channel: channels)
			for	(Client* member: channel->members)
				if (member != this)
					member->sendLine(":", nick, " NICK ", newNick);
	nick = newNick;

	if (!nickAlreadySubmitted)
		handleRegistrationComplete();
}
