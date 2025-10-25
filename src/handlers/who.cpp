#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "server.hpp"
#include "irc.hpp"
#include <cstring>

/**
 * Handle a WHO message.
 */
void Client::handleWho(int argc, char** argv)
{
	// Check that the correct number of parameters were given.
	if (argc > 2) {
		sendLine("461 ", nick, " WHO :Not enough parameters");
		return log::warn(nick, " WHO: Not enough parameters");
	}
	// If the server doesn't support the WHO command with a <mask> parameter, it
	// can send just an empty list.
	if (argc == 0) {

		// Send information about each client connected to the server.
		for (auto& [_, client]: server->allClients()) {

			// Get the name of one channel that this client is on, or '*' if the
			// client is not joined to any channels.
			std::string_view channel = "*";
			if (!client.channels.empty())
				channel = (*client.channels.begin())->name;

			// Send some information about the client.
			send("351 ", nick, " ");
			send(channel, " ");
			send(client.user, " ");
			send(client.host, " ");
			send(SERVER_NAME " ");
			send(client.nick, " ");
			send("H "); // Away status is not implemented.
			send(":0 "); // No server networks, so hop count is always zero.
			sendLine(realname);
		}
	}
	return sendLine("315 ", nick, " ", argv[0], " :End of WHO list");
}
