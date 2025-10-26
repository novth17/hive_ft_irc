#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "server.hpp"
#include "irc.hpp"
#include <cstring>

void Client::handlePrivMsg(int argc, char** argv)
{
	// Check the parameter count.
	if (!checkParams("PRIVMSG", true, argc, 1, 2))
		return;

	// Iterate over the list of message targets.
	char* targetList = argv[0];
	char* message = argv[1];
	while (*targetList != '\0') {

		// Check if the target is a channel.
		char* target = nextListItem(targetList);
		if (Channel::isValidName(target)) {

			// Check that the channel exists.
			Channel* channel = server->findChannelByName(target);
			if (channel == nullptr) {
				log::warn("PRIVMSG: No such channel: ", target);
				sendNumeric("404", target, " :Cannot send to channel");
				continue;
			}

			// Check that the sender is a member of the channel.
			if (!channel->findClientByName(nick)) {
				log::warn("PRIVMSG: Client ", nick, " is not a member of channel ", target);
				sendNumeric("404", target, " :Cannot send to channel (not a member)");
				continue;
			}

			// Broadcast the message to all channel members.
			for (Client* member: channel->allMembers())
				if (member != this)
					member->sendLine(":", fullname, " PRIVMSG ", target, " :", message);

		// Otherwise, the target is another client.
		} else {

			// Check that the recipient exists.
			Client* client = server->findClientByName(target);
			if (client == nullptr) {
				log::warn("PRIVMSG: No such nick: ", target);
				sendNumeric("401", target, " :No such nick/channel");
				continue;
			}

			// Send the message.
			client->sendLine(":", fullname, " PRIVMSG ", target, " :", message);
		}
	}
}
