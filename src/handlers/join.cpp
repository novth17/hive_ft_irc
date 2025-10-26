#include <cstring>

#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "server.hpp"
#include "irc.hpp"

/**
 * Handle a JOIN message.
 */
void Client::handleJoin(int argc, char** argv)
{
	if (!checkParams("JOIN", true, argc, 1, 2))
		return;

	// Check that the client is registered.
	if (!isRegistered) {
		log::warn(nick, " JOIN: User is not registered yet");
		return sendNumeric("451", ":You have not registered");
	}

	// If there's a single parameter "0" (without a '#' prefix), then PART all
	// channels instead.
	if (argc == 1 && std::strcmp(argv[0], "0") == 0) {

		// Remove the client from all channels, also notifying all channel
		// members (including the departing client).
		for (Channel* channel: channels) {
			for (Client* member: channel->allMembers())
				member->sendLine(":", fullname, " PART ", channel->getName(), " :");
			channel->removeMember(*this);
			log::info(nick, " left channel ", channel->getName());
		}
		return channels.clear();
	}

	// Join a comma-separated list of channels.
	char noKeys[] = ""; // Empty list used if no keys were given.
	char* names = argv[0]; // List of channel names ("#abc,#def")
	char* keys = argc == 2 ? argv[1] : noKeys; // List of keys ("key1,key2")
	while (*names != '\0') {

		// Get the next channel name and key in the comma-separated list.
		char* name = nextListItem(names);
		char* key = nextListItem(keys);

		// Check that a valid channel name was given.
		if (!Channel::isValidName(name)) {
			log::warn(nick, " JOIN: Invalid channel name: ", name);
			sendNumeric("403", name, " :No such channel");
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
		if (channel->getKey() != key) {
			log::warn(nick, " JOIN: Cannot join channel, channel's key not match");
			sendNumeric("475", name, " :Cannot join channel (+k)");
			continue;
		}

		// Issue an error if the channel member limit has been reached.
		if (channel->isFull()) {
			log::warn(nick, " JOIN: Cannot join channel, channel member limit has been reached");
			sendNumeric("471", name, " :Cannot join channel (+l)");
			continue;
		}

		// Issue an error if the channel is invite-only, and the client hasn't
		// been invited.
		if (channel->isInviteOnly() && !channel->isInvited(*this)) {
			log::warn(nick, " JOIN: Cannot join channel, channel is invite-only");
			sendNumeric("473", name, " :Cannot join channel (+i)");
			continue;
		}

		// Join the channel.
		channel->addMember(*this);
		channels.insert(channel);

		// Send a JOIN message to the joining client.
		sendLine(":", fullname, " JOIN ", name);
		log::info(nick, " JOIN: a JOIN message was sent to the joining client");

		// Send the topic (with timestamp) if there is one.
		if (!channel->hasTopic()) {
			sendNumeric("332", name, " :", channel->getTopic());
			sendNumeric("333", channel->getName(), " :", channel->getTopicChange());
			log::info(nick, " JOIN: Sent the topic");
		}

		// Send a list of members in the channel.
		send(":", server->getHostname(), " 353 ", fullname, " = ", name, " :");
		for (Client* member: channel->allMembers()) {
			const char* prefix = channel->isOperator(*member) ? "@" : "";
			send(prefix, member->nick, " ");
		}
		sendLine(); // Line break at the end of the member list.
		sendNumeric("366", name, " :End of /NAMES list");
		log::info("Sent a list of members in the channel");

		// Notify other members of the channel that someone joined.
		for (Client* member: channel->allMembers())
			if (member != this)
				member->sendLine(":", fullname, " JOIN ", channel->getName());
	}
}
