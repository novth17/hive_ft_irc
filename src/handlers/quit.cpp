#include <cstring>

#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "server.hpp"
#include "irc.hpp"

/**
 * Handle a QUIT message.
 */
void Client::handleQuit(int argc, char** argv)
{
	// Check that enough parameters were provided.
	if (argc > 1) {
		log::warn(nick, "QUIT: Has to be 1 param: [<Quit message>]");
		return sendNumeric("461", "QUIT :Not enough parameters");
	}

	// Use a default message if none was provided.
	std::string_view reason = "Client exited the server";
	if (argc == 1 && std::strlen(argv[0]) != 0)
		reason = argv[0];

	log::info("QUIT: ", nick, " has quit");
	server->disconnectClient(*this, reason);
}
