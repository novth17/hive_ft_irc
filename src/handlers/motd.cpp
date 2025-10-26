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
		return sendNumeric("461", "MOTD :Not enough parameters");
	}

	// Server networks are not supported, so a <server> argument is always an
	// error.
	if (argc == 1)
		return sendNumeric("402", argv[0], " :No such server");

	// Send the message of the day one line at a time.
	sendNumeric("375", ":- " SERVER_NAME " Message of the day - ");
	for (const char* line = message; *line != '\0';) {
		size_t length = std::strcspn(line, "\n");
		sendNumeric("372", ":", std::string_view(line, length));
		line += length + (line[length] == '\n');
	}
	sendNumeric("376", ":End of /MOTD command.");
}
