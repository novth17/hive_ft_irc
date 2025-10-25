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

	if (isRegistered) {
		sendLine("462 ", nick.empty() ? "*" : nick, " :You may not reregister");
		return log::warn(nick, "PASS: Already registered user");
	}

	if (argc != 1){
		sendLine("461 ", nick.empty() ? "*" : nick, " PASS :Not enough parameters");
		return log::warn(nick, "PASS: Has to be 1 param: <password>");
	}

	if (server->correctPassword(argv[0]) == false)
	{
		isPassValid = false;
		sendLine("464 ", nick, " :Password incorrect");
		log::warn(nick, "PASS: Password is incorrect");
		return server->disconnectClient(*this, "Incorrect password");
	}

	isPassValid = true;

	if (!passAlreadyValid)
		handleRegistrationComplete();
}
