#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "server.hpp"
#include "irc.hpp"
#include <cstring>

/**
 * Handle PrivMsgParams.
 */
bool Client::handlePrivMsgParams(int argc, char** argv) {

	(void) argv;

	if (argc < 1) {
		sendNumeric("411", ":No recipient given");
		log::warn("PRIVMSG: ", "No recipient parameter. NICK: ", nick);
		return false;
	}

	if (argc < 2) {
		sendNumeric("412", ":No text to send");
		log::warn("PRIVMSG: ", "No recipient parameter. NICK: ", nick);
		return false;
	}
	return true;
}

void Client::handlePrivMsg(int argc, char** argv)
{
	if (handlePrivMsgParams(argc, argv) == false)
		return;

	// Iterate over the list of message targets.
	char* targetList = argv[0];
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

			// Broadcast the message to all channel members. can make this a  method
			for (Client* member: channel->members) {
				if (member != this) {
					member->send(":", fullname, " PRIVMSG ", target, " :");
					for (int i = 1; i < argc; i++)
						member->send(i == 1 ? "" : " ", argv[i]);
					member->sendLine();
				}
			}

		// Otherwise, the target is another client.
		} else {
			// Check that the client is registered.
			if (!isRegistered) {
				sendLine("451 ", nick, " :You have not registered");
				log::warn(nick, " PRIVMSG: User is not registered yet");
				continue;
			}
			Client* client = server->findClientByName(target);
			if (client == nullptr) {
				log::warn("PRIVMSG: No such nick: ", target);
				sendNumeric("401", target, " :No such nick/channel");
				continue;
			}

			// Send all parts of the message.
			client->send(":", fullname, " PRIVMSG ", target, " :");
			for (int i = 1; i < argc; i++)
				client->send(i == 1 ? "" : " ", argv[i]);
			client->sendLine();
		}
	}
}
