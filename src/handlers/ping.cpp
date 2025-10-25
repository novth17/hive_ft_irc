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
	//FIXME: Empty token: ERR_NOORIGIN (409) "<client> :No origin specified"
	if (argc != 1) {
		sendLine("461 ", nick, " PING :Not enough parameters");
		return log::warn(nick, "PING: Has to be 1 param: <server>");
	}
	// Send the token back to the client in a PONG message.
	sendLine(":localhost PONG :", argv[0]); // FIXME: What should the source be?
}
