#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "server.hpp"
#include "irc.hpp"

/**
 * Handle a PASS message.
 */
void Client::handlePass(int argc, char** argv)
{
	if (isRegistered) {
		log::warn(nick, " PASS: Already registered user");
		return sendNumeric("462", ":You may not reregister");
	}

	if (argc != 1) {
		log::warn(nick, " PASS: Has to be 1 param: <password>");
		return sendNumeric("461", "PASS :Not enough parameters");
	}

	if (!server->correctPassword(argv[0])) {
		isPassValid = false;
		sendNumeric("464", ":Password incorrect");
		log::warn(nick, " PASS: Password is incorrect");
		return server->disconnectClient(*this, "Incorrect password");
	}

	bool passAlreadyValid = isPassValid;
	isPassValid = true;
	if (!passAlreadyValid)
		handleRegistrationComplete();
}
