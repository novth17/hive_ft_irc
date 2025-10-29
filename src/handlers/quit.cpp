#include <cstring>

#include "client.hpp"
#include "log.hpp"
#include "server.hpp"

/**
 * Handle a QUIT message.
 */
void Client::handleQuit(int argc, char** argv)
{
	// Check that enough parameters were provided.
	if (!checkParams("QUIT", false, argc, 0, 1))
		return;

	// Use a default message if none was provided.
	std::string_view reason = "Client exited the server";
	if (argc == 1 && std::strlen(argv[0]) != 0)
		reason = argv[0];

	// Disconnect the client with the reason message.
	log::info("QUIT: ", nick, " has quit");
	server.disconnectClient(*this, reason);
}
