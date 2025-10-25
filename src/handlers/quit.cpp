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
	std::string_view reason;

	if (argc >= 1 && argv && argv[0] && *argv[0]) //first arg ptr not null and string isnt empty
		reason = (argv[0]);
	else
		reason = "Client exited the server";

	log::info("QUIT: ", nick, " has quit");
	server->disconnectClient(*this, reason);
}
