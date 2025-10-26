#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "server.hpp"

/*
 * Handle a NOTICE message. This is basically the same as PRIVMSG, except that
 * no errors are sent back to the sending client.
 */
void Client::handleNotice(int argc, char** argv)
{
	// Check registration and parameters, but without sending error messages.
	if (!isRegistered || argc < 1 || argc > 2)
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
				log::warn("NOTICE: No such channel: ", target);
				continue;
			}

			// Check that the sender is a member of the channel.
			if (!channel->findClientByName(nick)) {
				log::warn("NOTICE: Client ", nick, " is not a member of channel ", target);
				continue;
			}

			// Broadcast the message to all channel members.
			for (Client* member: channel->allMembers())
				if (member != this)
					member->sendLine(":", fullname, " NOTICE ", target, " :", message);

		// Otherwise, the target is another client.
		} else {

			// Check that the recipient exists.
			Client* client = server->findClientByName(target);
			if (client == nullptr) {
				log::warn("NOTICE: No such nick: ", target);
				continue;
			}

			// Send the message.
			client->sendLine(":", fullname, " NOTICE ", target, " :", message);
		}
	}
}
