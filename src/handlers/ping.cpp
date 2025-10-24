#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "server.hpp"
#include "irc.hpp"
#include <cstring>

/**
 * Handle a PING message.
 */
void Client::handlePing(int argc, char** argv)
{
	// Check that enough parameters were provided.
	if (argc != 1)
		return sendLine("461 ", nick, " PING :Not enough parameters");

	// Send the token back to the client in a PONG message.
	sendLine(":localhost PONG :", argv[0]); // FIXME: What should the source be?
}
