#include "channel.hpp"
#include "client.hpp"
#include "server.hpp"
#include "utility.hpp"

/**
 * Handle a NAMES command.
 */
void Client::handleNames(int argc, char** argv)
{
	// Check that enough parameters were given.
	if (argc != 1)
		return sendNumeric("461", "NAMES :Not enough parameters");

	// Traverse the comma-separated list of channels.
	char* channelList = argv[0];
	while (*channelList != '\0') {

		// Only list clients if the channel exists.
		char* channelName = nextListItem(channelList);
		Channel* channel = server->findChannelByName(channelName);
		if (channel != nullptr) {

			// List the channel's members.
			send(":", server->getHostname(), " 353 ", fullname, " = ", channelName, " :");
			for (Client* member: channel->members) {
				const char* prefix = channel->isOperator(*member) ? "@" : "";
				send(prefix, member->nick, " ");
			}
			sendLine(); // End the RPL_NAMREPLY (353) numeric.
		}

		// Send an end-of-names numeric either way.
		sendNumeric("366", channelName, " :End of /NAMES list");
	}
}
