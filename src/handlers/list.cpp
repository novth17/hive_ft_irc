#include "channel.hpp"
#include "client.hpp"
#include "server.hpp"
#include "utility.hpp"

/**
 * Handle a LIST command.
 */
void Client::handleList(int argc, char** argv)
{
	// Check that enough parameters were given.
	if (argc > 1)
		return sendNumeric("461", "LIST :Not enough parameters");

	// The list start reply is always sent.
	sendNumeric("321", "Channel :Users  Name");

	// If no parameters were given, list all channels.
	if (argc == 0) {
		for (auto& [_, channel]: server->allChannels()) {
			send(":", server->getHostname(), " 322 ", nick, " ", channel.name);
			sendLine(" ", channel.getMemberCount(), " :", channel.topic);
		}

	// Otherwise, list just the info for the listed channels.
	} else if (argc == 1) {
		char* channelList = argv[0];
		while (*channelList != '\0') {
			char* channelName = nextListItem(channelList);
			Channel* channel = server->findChannelByName(channelName);
			if (channel != nullptr) {
				send(":", server->getHostname(), " 322 ");
				send(fullname, " ", channel->name, " ");
				sendLine(channel->getMemberCount(), " :", channel->topic);
			}
		}
	}

	// The list end reply is always sent.
	sendNumeric("323", ":End of /LIST");
}
