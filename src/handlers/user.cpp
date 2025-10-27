#include <cstring>

#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "irc.hpp"

/**
 * Handle a USER message.
   USER <username> 0 * <realname>
 */
void Client::handleUser(int argc, char** argv)
{
	// Check parameter count.
	if (!checkParams("USER", false, argc, 4, 4))
		return;

 	// Must have passed the correct password first: https://datatracker.ietf.org/doc/html/rfc2812#section-3.1.1
	if (!isPassValid) {
		log::warn(user, " USER: Password is not yet set");
		return sendNumeric("464", ":Password incorrect");
	}

	// Don't allow the user to reregister.
	if (isRegistered) {
		log::warn(nick, " USER: Already registered user tried USER again");
		return sendNumeric("462", ":You may not reregister");
	}


	// Save username and real name
	bool userAlreadySubmitted = !user.empty() || !realname.empty();
	user = argv[0];
	if (user.length() == 0) {
		log::warn(nick, " USER: Attempted to register with empty user string");
		return sendNumeric("461", "USER", " :Not enough parameters");
	}
	if (user.length() > USERLEN)
		user.resize(USERLEN);
	realname = argv[3];
	fullname = nick + "!" + user + "@" + host;

	log::info(nick, " registered USER as ", user, " (realname: ", realname, ")");
	if (!userAlreadySubmitted)
		handleRegistrationComplete();
}
