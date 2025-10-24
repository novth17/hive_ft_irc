#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "server.hpp"
#include "irc.hpp"
#include <cstring>

/**
 * Handle a USER message.
 */
void Client::handleUser(int argc, char** argv)
{
	if (!isPassValid) { // Must have passed the correct password first: https://datatracker.ietf.org/doc/html/rfc2812#section-3.1.1
		sendLine("464 :Password incorrect");
		return log::warn(user, "Password is not yet set");
	}

	if (isRegistered) {
		sendLine("462 :You may not reregister");
		return log::warn(nick, "Already registered user tried USER again");
	}

	if (argc < 4 || argv[0] == NULL || argv[3] == NULL) // USER <username> <0> <*> <realname>
		return sendLine("461 USER :Not enough parameters");

	bool userAlreadySubmitted = (!user.empty() || !realname.empty()); // FIXME: Is it possible for us to receive them empty?

	// Save username and real name
	user = argv[0];
	realname = argv[3];

	log::info(nick, " registered USER as ", user, " (realname: ", realname, ")");

	if (!userAlreadySubmitted)
		handleRegistrationComplete();
}
