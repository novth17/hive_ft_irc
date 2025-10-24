#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "server.hpp"
#include "irc.hpp"
#include <cstring>

/**
 * Handle a JOIN message.
 */
void Client::handleJoin(int argc, char** argv)
{
	// Check that enough parameters were provided.
	if (argc < 1 || argc > 2)
		return sendLine("461 ", nick, " JOIN :Not enough parameters");

	// Check that the client is registered.
	if (!isRegistered)
		return sendLine("451 ", nick, " :You have not registered");

	// Join a list of channels.
	char noKeys[] = ""; // Empty list used if no keys were given.
	char* names = argv[0]; // List of channel names ("#abc,#def")
	char* keys = argc == 2 ? argv[1] : noKeys; // List of keys ("key1,key2")
	while (*names != '\0') {

		// Get the next channel name and key in the comma-separated list.
		char* name = nextListItem(names);
		char* key = nextListItem(keys);

		// Check that a valid channel name was given.
		if (!Channel::isValidName(name)) {
			sendLine("403 ", nick, " ", name, " :No such channel");
			continue;
		}

		// If there's no channel by that name, create it.
		Channel* channel = server->findChannelByName(name);
		if (channel == nullptr)
			channel = server->newChannel(name);

		// Skip if the client is already in the channel.
		if (channel->findClientByName(nick) != nullptr)
			continue;

		// Issue an error message if the key doesn't match.
		if (channel->key != key) {
			sendLine("475 ", nick, " ", name, " :Cannot join channel (+k)");
			continue;
		}

		// Issue an error if the channel member limit has been reached.
		if (channel->isFull()) {
			sendLine("471 ", nick, " ", name, " :Cannot join channel (+l)");
			continue;
		}

		// Issue an error if the channel is invite-only, and the client hasn't
		// been invited.
		if (channel->inviteOnly) { // FIXME: Check for invite
			sendLine("473 ", nick, " ", name, " :Cannot join channel (+i)");
			continue;
		}

		// Join the channel.
		channel->addMember(*this);

		// Send a JOIN message to the joining client.
		sendLine(":", fullname, " JOIN ", name);

		// Send the topic (with timestamp) if there is one.
		if (!channel->topic.empty()) {
			sendLine("332 ", nick, " ", name, " :", channel->topic);
			sendLine("333 ", nick, " ", channel->name, " ", channel->topicChangeStr);
		}

		// Send a list of members in the channel.
		send("353 ", nick, " ", channel->symbol, " ", name, " :");
		for (Client* member: channel->members) {
			const char* prefix = channel->isOperator(*member) ? "@" : "";
			send(prefix, member->nick, " ");
		}
		sendLine(); // Line break at the end of the member list.
		sendLine("366 ", nick, " ", name, " :End of /NAMES list");

		// Notify other members of the channel that someone joined.
		for (Client* member: channel->members)
			if (member != this)
				member->sendLine(":", fullname, " JOIN ", channel->name);
	}
}
