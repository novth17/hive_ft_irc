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
	// Check that enough parameters were provided.
	if (argc != 1)
		return sendLine("461 ", nick, " QUIT :Not enough parameters");

	// The <reason> for disconnecting must be prefixed with "Quit:" when sent
	// from the server.
	std::string reason = "Quit: " + std::string(argv[0]);
	server->disconnectClient(*this, reason);
}
