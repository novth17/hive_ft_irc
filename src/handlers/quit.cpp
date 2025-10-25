#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "server.hpp"
#include "irc.hpp"
#include <cstring>

/**
 * Handle a QUIT message.
 */
void Client::handleQuit(int argc, char** argv)
{
	//@FIXME add default message when doing just QUIT
	// Check that enough parameters were provided.
	if (argc != 1) {
		sendLine("461 ", nick, " QUIT :Not enough parameters");
		return log::warn(nick, "QUIT: Has to be 1 param: [<Quit message>]");
	}

	// The <reason> for disconnecting must be prefixed with "Quit:" when sent
	// from the server.
	std::string reason = "Quit: " + std::string(argv[0]);
	log::info("QUIT: ", nick, " has quit");
	server->disconnectClient(*this, reason);
}
