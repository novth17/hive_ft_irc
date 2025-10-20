#include <string.h>
#include <sys/socket.h>

#include "channel.hpp"
#include "client.hpp"
#include "irc.hpp"
#include "log.hpp"
#include "server.hpp"
#include "utility.hpp"

/**
 * Send a string of text to the client. All the other variants of the
 * Client::send method call this one to do their business.
 */
void Client::send(const std::string_view& string)
{
	output.append(string);
	ssize_t bytes = 1;
	while (bytes > 0 && output.find("\r\n") != output.npos) {
		bytes = ::send(socket, output.data(), output.size(), 0);
		if (bytes == -1) {
			if (errno == EAGAIN)
				break;
			fail("Failed to send to client: ", strerror(errno));
		}
		output.erase(0, bytes);
	}
}

/**
 * Handle a USER message.
 */
void Client::handleUser(int argc, char** argv)
{
	(void) argc, (void) argv;
	log::warn("Unimplemented command: USER");
}

/**
 * Handle a NICK message.
 */
void Client::handleNick(int argc, char** argv)
{
	(void) argc, (void) argv;
	log::warn("Unimplemented command: NICK");
}

/**
 * Handle a PASS message.
 */
void Client::handlePass(int argc, char** argv)
{
	// TODO: Implement 462 ?
	if (argc != 1)
		return sendLine("461 ", nick, " JOIN :Not enough parameters");
	if (server->correctPassword(argv[0]) == false)
		return sendLine("464 ", nick, " JOIN :Password incorrect");
	authorized = true;
	// TODO: Do I need to implement a success message?
}

/**
 * Handle a CAP message.
 */
void Client::handleCap(int argc, char** argv)
{
	(void) argc, (void) argv;
	log::warn("Unimplemented command: CAP");
}

/**
 * Handle a PART message.
 */
void Client::handlePart(int argc, char** argv)
{
	// Check that one or two parameters were provided.
	if (argc < 1 || argc > 2)
		return sendLine("461 ", nick, " JOIN :Not enough parameters");

	// Check that the client is registered.
	if (!isRegistered)
		return sendLine("451 ", nick, " :You have not registered");

	// Iterate over the list of channels to leave.
	char* channelNameList = argv[0];
	std::string reason = argc == 2 ? " :" + std::string(argv[1]) : "";
	while (*channelNameList != '\0') {

		// Check that the channel exists.
		char* channelName = nextListItem(channelNameList);
		Channel* channel = server->findChannelByName(channelName);
		if (channel == nullptr) {
			sendLine("403 ", nick, " ", channelName, " :No such channel");
			continue;
		}

		// Check that the client is actually on that channel.
		if (channels.find(channel) == channels.end()) {
			sendLine("442 ", nick, " ", channelName, " :You're not on that channel");
			continue;
		}

		// Leave the channel and send a PART message to the client.
		leaveChannel(channel);
		sendLine("PART ", channel->name, reason);

		// Send PART messages to all members of the channel, with the departed
		// client's nickname as the <source>.
		for (Client* member: channel->members)
			member->sendLine(":", nick, " PART", channel->name, reason);
	}
}

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
			sendLine(nick, " ", name, " :Cannot join channel (+k)");
			continue;
		}

		// Join the channel.
		joinChannel(channel);

		// Send a join message, the topic, and a list of channel members.
		sendLine(":", nick, " JOIN ", name);
		sendLine("332 ", nick, " ", name, " :", channel->topic);
		send("353 ", nick, " ", channel->symbol, " ", name, " :");
		for (Client* member: channel->members)
			send(member->prefix, member->nick, " ");
		sendLine(); // Line break at the end of the member list.
		sendLine("366 ", nick, " ", name, " :End of /NAMES list");
	}
}

/**
 * Makes a client a member of a channel without checking for authorization. Does
 * nothing if the client is already joined to the channel.
 */
void Client::joinChannel(Channel* channel)
{
	assert(channel != nullptr);
	assert(channels.contains(channel) == !!channel->findClientByName(nick));
	if (channels.contains(channel)) {
		log::warn(nick, " is already in channel ", channel->name);
	} else {
		channel->members.insert(this);
		channels.insert(channel);
		log::info(nick, " was added to channel ", channel->name);
	}
	assert(channels.find(channel) != channels.end());
	assert(channel->findClientByName(nick) == this);
}

/**
 * Makes a client no longer a member of of a channel. Does nothing if the client
 * is not a member of the channel.
 */
void Client::leaveChannel(Channel* channel)
{
	assert(channel != nullptr);
	assert(channels.contains(channel) == !!channel->findClientByName(nick));
	if (channels.contains(channel)) {
		channel->members.erase(this);
		channels.erase(channel);
		log::info(nick, " was removed from channel ", channel->name);
	} else {
		log::warn(nick, " is already in channel ", channel->name);
	}
	assert(channels.find(channel) == channels.end());
	assert(channel->findClientByName(nick) == nullptr);
}
