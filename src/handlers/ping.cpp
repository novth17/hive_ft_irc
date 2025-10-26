#include "client.hpp"
#include "log.hpp"
#include "server.hpp"

/**
 * Handle a PING message.
 */
void Client::handlePing(int argc, char** argv)
{
	// Check that enough parameters were provided.
	if (argc != 1) {
		log::warn(nick, "PING: Has to be 1 param: <server>");
		return sendNumeric("461", "PING :Not enough parameters");
	}

	// Check that the token isn't empty.
	char* token = argv[0];
	if (*token == '\0')
		return sendNumeric("409", ":No origin specified");

	// Send the token back to the client in a PONG message.
	sendLine(":", server->getHostname(), " PONG :", token);
}
