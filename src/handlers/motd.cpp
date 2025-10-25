#include <cstring>

#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "server.hpp"
#include "irc.hpp"

// The hard-coded message of the day to send.
static const char* message = R"(
██╗██████╗  ██████╗███████╗██╗   ██╗███████╗███████╗███████╗██████╗ 
██║██╔══██╗██╔════╝██╔════╝██║   ██║██╔════╝██╔════╝██╔════╝██╔══██╗
██║██████╔╝██║     ███████╗██║   ██║█████╗  █████╗  █████╗  ██████╔╝
██║██╔══██╗██║     ╚════██║██║   ██║██╔══╝  ██╔══╝  ██╔══╝  ██╔══██╗
██║██║  ██║╚██████╗███████║╚██████╔╝██║     ██║     ███████╗██║  ██║
╚═╝╚═╝  ╚═╝ ╚═════╝╚══════╝ ╚═════╝ ╚═╝     ╚═╝     ╚══════╝╚═╝  ╚═╝
)";

/**
 * Handle an MOTD message.
 */
void Client::handleMotd(int argc, char** argv)
{
	// Check parameters.
	if (argc > 1) {
		log::warn(nick, "MOTD: Has to be 0 or 1 params");
		return sendLine("461 ", nick, " MOTD :Not enough parameters");
	}

	// Server networks are not supported, so a <server> argument is always an
	// error.
	if (argc == 1)
		return sendLine("402 ", fullname, " ", argv[0], " :No such server");

	// Send the message of the day one line at a time.
	sendLine("375 ", fullname, " :- " SERVER_NAME " Message of the day - ");
	for (const char* line = message; *line != '\0';) {
		size_t length = std::strcspn(line, "\n");
		sendLine("372 ", fullname, " :", std::string_view(line, length));
		line += length + (line[length] == '\n');
	}
	sendLine("376 ", fullname, " :End of /MOTD command.");
}
