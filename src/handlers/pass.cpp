#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "server.hpp"
#include "irc.hpp"
#include <cstring>

/**
 * Handle a PASS message.
 */
void Client::handlePass(int argc, char** argv)
{
	bool passAlreadyValid = isPassValid;

	if (isRegistered)
		return sendLine("462 ", nick.empty() ? "*" : nick, " :You may not reregister");
	if (argc != 1)
		return sendLine("461 ", nick.empty() ? "*" : nick, " PASS :Not enough parameters");
	if (server->correctPassword(argv[0]) == false)
	{
		isPassValid = false;
		sendLine("464 ", nick, " :Password incorrect");
		return server->disconnectClient(*this, "Incorrect password");
	}
	isPassValid = true;

	if (!passAlreadyValid)
		handleRegistrationComplete();
}
