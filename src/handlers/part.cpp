#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "server.hpp"
#include "irc.hpp"
#include <cstring>

/**
 * Handle a PART message.
 */
void Client::handlePart(int argc, char** argv)
{
	// Check that one or two parameters were provided.
	if (argc < 1 || argc > 2) {
		log::warn(nick, " PART: Need more params or too many params: <channel>{,<channel>}");
		return sendNumeric("461", "PART :Not enough parameters");
	}

	// Check that the client is registered.
	if (!isRegistered) {
		log::warn(nick, " PART: User is not registered yet");
		return sendNumeric("451", ":You have not registered");
	}

	// Iterate over the list of channels to leave.
	char* channelNameList = argv[0];
	std::string reason = argc == 2 ? " :" + std::string(argv[1]) : "";
	while (*channelNameList != '\0') {

		// Check that the channel exists.
		char* channelName = nextListItem(channelNameList);
		Channel* channel = server->findChannelByName(channelName);
		if (channel == nullptr) {
			sendNumeric("403", channelName, " :No such channel");
			log::warn(nick, " PART: Invalid channel name");
			continue;
		}

		// Check that the client is actually on that channel.
		if (!channel->isMember(*this)) {
			sendNumeric("442", channelName, " :You're not on that channel");
			log::warn(nick, " PART: You're not on channel: ", channelName);
			continue;
		}

		// Leave the channel and send a PART message to the client.
		channel->removeMember(*this);
		channels.erase(channel);
		sendLine(":", fullname, " PART ", channel->name);

		// Send PART messages to all members of the channel, with the departed
		// client's nickname as the <source>.
		for (Client* member: channel->members)
			member->sendLine(":", fullname, " PART ", channel->name, reason);
		log::info(nick, " left channel ", channel->name);
	}
}
