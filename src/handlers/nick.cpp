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
		return log::warn(user, "Password is not yet set");
	}

	if (argc == 0)
		return sendLine("431 ", nick, " :No nickname given");
	if (argc > 2)
		return sendLine("461 ", nick, " NICK :Not enough parameters");

	bool nickAlreadySubmitted = !nick.empty();
	std::string_view newNick = argv[0];

	if (server->findClientByName(newNick))
		return sendLine("433 ", nick, " ", newNick, " :Nickname is already in use");
	if ((newNick[0] == ':') || (newNick[0] == '#')
		|| std::string_view(newNick).find(' ') != std::string::npos)
		return sendLine("432 ", nick, " ", newNick, " :Erroneus nickname");

	if (isRegistered)
		for (Channel* channel: channels)
			for	(Client* member: channel->members)
				if (member != this)
					member->sendLine(":", nick, " NICK ", newNick);
	nick = newNick;

	if (!nickAlreadySubmitted)
		handleRegistrationComplete();
}
