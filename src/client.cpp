#include <string.h>
#include <sys/socket.h>
#include <iomanip>

#include "channel.hpp"
#include "client.hpp"
#include "irc.hpp"
#include "log.hpp"
#include "server.hpp"
#include "utility.hpp"

#define SUCCESS 0
#define FAIL 1

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
	if (!isPassValid) { // Must have passed the correct password first: https://datatracker.ietf.org/doc/html/rfc2812#section-3.1.1
		sendLine("464 :Password incorrect");
		return log::warn(user, "Password is not yet set");
	}

	if (isRegistered) {
		sendLine("462 :You may not reregister");
		return log::warn(nick, "Already registered user tried USER again");
	}

	if (argc < 4 || argv[0] == NULL || argv[3] == NULL) // USER <username> <0> <*> <realname>
		return sendLine("461 USER :Not enough parameters");

	bool userAlreadySubmitted = (!user.empty() || !realname.empty()); // FIXME: Is it possible for us to receive them empty?

	// Save username and real name
	user = argv[0];
	realname = argv[3];
	if (realname[0] == ':')
		realname.erase(0, 1); // remove the ':' prefix if present

	log::info(nick, " registered USER as ", user, " (realname: ", realname, ")");

	if (!userAlreadySubmitted)
		handleRegistrationComplete();
}

/**
 * Handle a NICK message.
 */
void Client::handleNick(int argc, char** argv)
{
	if (!isPassValid) { // Must have passed the correct password first: https://datatracker.ietf.org/doc/html/rfc2812#section-3.1.1
		sendLine("464 :Password incorrect");
		return log::warn(user, "Password is not yet set");
	}

	if (argc == 0)
		return sendLine("431 ", nick, " :No nickname given");
	if (argc > 2)
		return sendLine("461 ", nick, " NICK :Not enough parameters");
	
	bool nickAlreadySubmitted = !nick.empty();
	std::string_view newNick = argv[0];

	if (server->findClientByName(newNick))
		return sendLine("433 ", nick, " ", newNick, " :Nickname is already in use");
	if ((newNick[0] == ':') || (newNick[0] == '#')
		|| std::string_view(newNick).find(' ') != std::string::npos)
		return sendLine("432 ", nick, " ", newNick, " :Erroneus nickname");

	if (isRegistered)
		for (Channel* channel: channels)
			for	(Client* member: channel->members)
				if (member != this)
					member->sendLine(":", nick, " NICK ", newNick);
	nick = newNick;

	if (!nickAlreadySubmitted)
		handleRegistrationComplete();
}

/**
 * Handle a PASS message.
 */
void Client::handlePass(int argc, char** argv)
{
	bool passAlreadyValid = isPassValid;

	if (isRegistered)
		return sendLine("462 ", nick.empty() ? "*" : nick, " :You may not reregister");
	if (argc != 1)
		return sendLine("461 ", nick.empty() ? "*" : nick, " PASS :Not enough parameters");
	if (server->correctPassword(argv[0]) == false)
	{
		isPassValid = false;
		sendLine("464 ", nick, " :Password incorrect");
		return server->disconnectClient(*this, "Incorrect password");
	}
	isPassValid = true;

	if (!passAlreadyValid)
		handleRegistrationComplete();
	// TODO: Message for when password is already valid and resubmitted?
}

void Client::handleRegistrationComplete()
{
	if (!nick.empty() && !user.empty() && isPassValid)
		isRegistered = true;

	if (isRegistered)
	{
		time_t _tm = time(NULL);
		struct tm* curtime = localtime(&_tm);

		sendLine("001 ", nick, " :Welcome to the ", SERVER_NAME, " Network ", nick, "!", user, "@localhost");
		sendLine("002 ", nick, " :Your host is ", SERVER_NAME, ", running version 1.0");
		send("003 ", nick, " :This server was created ", asctime(curtime)); // asctime() has a newline at the end
		sendLine("004 ", nick, " ", SERVER_NAME, " Version 1.0");
	}
}

/**
 * Handle a CAP message.
 */
void Client::handleCap(int argc, char** argv)
{
	(void) argc, (void) argv;

	// If the server doesn't support the CAP command, then no specific response
	// is expected by the client, so we just do nothing here.
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
 * Handle a PING message.
 */
void Client::handlePing(int argc, char** argv)
{
	// Check that enough parameters were provided.
	if (argc != 1)
		return sendLine("461 ", nick, " JOIN :Not enough parameters");

	// Send the token back to the client in a PONG message.
	sendLine(":localhost PONG :", argv[0]); // FIXME: What should the source be?
}

/**
 * Handle a QUIT message.
 */
void Client::handleQuit(int argc, char** argv)
{
	// Check that enough parameters were provided.
	if (argc != 1)
		return sendLine("461 ", nick, " JOIN :Not enough parameters");

	// The <reason> for disconnecting must be prefixed with "Quit:" when sent
	// from the server.
	std::string reason = "Quit: " + std::string(argv[0]);
	server->disconnectClient(*this, reason);
}

/**
 * Handle a MODE message.
 */
void Client::handleMode(int argc, char** argv)
{
	// Check that enough parameters were provided.
	if (argc < 1 || argc > 3)
		return sendLine("461 ", nick, " JOIN :Not enough parameters");

	// Check if the target is a channel.
	char* target = argv[0];
	if (Channel::isValidName(target)) {

		// Check that the channel actually exists.
		Channel* channel = server->findChannelByName(target);
		if (channel == nullptr)
			return sendLine("403 ", nick, " ", target, " :No such channel");

		// If no mode string was given, reply with the channel's current modes.
		if (argc < 2)
			return sendLine("324 ", nick, " ", target, " :", channel->modes);

		log::error("MODE <channel> <modestring> is not yet implemented");
		// TODO: Parse the mode string and change the channel mode.

	// Otherwise, the target must be a client.
	} else {

		// Check that the client exists.
		Client* client = server->findClientByName(target);
		if (client == nullptr)
			return sendLine("401 ", nick, " ", target, " :No such nick/channel");

		// Check that the target matches the client's own nickname.
		if (client->nick != target)
			return sendLine("502 ", nick, " :Cant change mode for other users");

		// If no mode string was given, reply with the client's current modes.
		if (argc < 2)
			return sendLine("221 ", nick, " :", modes);

		log::error("MODE <client> <modestring> is not yet implemented");
		// TODO: Parse the mode string and change the user mode.
	}
}

/**
 * Handle a WHO message.
 */
void Client::handleWho(int argc, char** argv)
{
	// Check that the correct number of parameters were given.
	if (argc > 2)
		return sendLine("461 ", nick, " JOIN :Not enough parameters");

	// If the server doesn't support the WHO command with a <mask> parameter, it
	// can send just an empty list.
	if (argc == 0) {

		// Send information about each client connected to the server.
		for (auto& [_, client]: server->allClients()) {
			auto channel = client.channels.begin();
			send("351 ", nick, " ");
			send(channel == client.channels.end() ? "*" : (*channel)->name, " ");
			send(client.user, " localhost ircserv ");
			send("localhost "); // FIXME: Use the actual client hostname.
			send(SERVER_NAME " ");
			send(client.nick, " ");
			send("H "); // Away status is not implemented.
			send(":0 "); // No server networks, so hop count is always zero.
			sendLine(realname);
		}
	}
	return sendLine("315 ", nick, " ", argv[0], " :End of WHO list");
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
