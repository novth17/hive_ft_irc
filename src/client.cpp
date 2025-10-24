#include <string.h>
#include <string_view>
#include <cstring>
#include <string.h>
#include <string_view>
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
		bytes = ::send(socket, output.data(), output.size(), MSG_DONTWAIT);
		if (bytes == -1) {
			if (errno == EAGAIN || errno == ECONNRESET)
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
}

void Client::handleRegistrationComplete()
{
	if (nick.empty() || user.empty() || !isPassValid)
		return;
	isRegistered = true;
	fullname = nick + "!" + user + "@" + host;
	sendLine("001 ", nick, " :Welcome to the ", SERVER_NAME, " Network ", fullname);
	sendLine("002 ", nick, " :Your host is ", SERVER_NAME, ", running version 1.0");
	sendLine("003 ", nick, " :This server was created ", server->getLaunchTime());
	sendLine("004 ", nick, " ", SERVER_NAME, " Version 1.0");
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
		return sendLine("461 ", nick, " PART :Not enough parameters");

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
		if (!channel->isMember(*this)) {
			sendLine("442 ", nick, " ", channelName, " :You're not on that channel");
			continue;
		}

		// Leave the channel and send a PART message to the client.
		channel->removeMember(*this);
		sendLine(":", fullname, " PART ", channel->name);

		// Send PART messages to all members of the channel, with the departed
		// client's nickname as the <source>.
		for (Client* member: channel->members)
			member->sendLine(":", fullname, " PART ", channel->name, reason);
		log::info(nick, " left channel ", channel->name);
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
			sendLine("475 ", nick, " ", name, " :Cannot join channel (+k)");
			continue;
		}

		// Issue an error message if the channel is invite-only and the client isn't invited
		if (channel->inviteOnly && !channel->isInvited(nick)) {
			sendLine("473 ", nick, " ", channel->name, " :Cannot join channel (+i)");
			continue;
		}

		// Issue an error if the channel member limit has been reached.
		if (channel->isFull()) {
			sendLine("471 ", nick, " ", name, " :Cannot join channel (+l)");
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

/**
 * Handle a PING message.
 */
void Client::handlePing(int argc, char** argv)
{
	// Check that enough parameters were provided.
	if (argc != 1)
		return sendLine("461 ", nick, " PING :Not enough parameters");

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
		return sendLine("461 ", nick, " QUIT :Not enough parameters");

	// The <reason> for disconnecting must be prefixed with "Quit:" when sent
	// from the server.
	std::string reason = "Quit: " + std::string(argv[0]);
	server->disconnectClient(*this, reason);
}

/**
 * Handle PrivMsgParams.
 */
bool Client::handlePrivMsgParams(int argc, char** argv) {

	(void) argv;

	if (argc < 1) {
		sendLine("411 " , nick, " :No recipient given");
		log::warn("PRIVMSG: ", "No recipient parameter. NICK: ", nick);
		return false;
	}

	if (argc < 2) {
		sendLine("412 " , nick, " :No text to send");
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
				sendLine("403 ", nick, " ", target, " :No such channel");
				log::warn("PRIVMSG: No such channel: ", target);
				continue;
			}

			// Check that the sender is a member of the channel.
			if (!channel->findClientByName(nick)) {
				sendLine("404 ", nick, " ", target, " :Cannot send to channel (not a member)");
				log::warn("PRIVMSG: Client ", nick, " is not a member of channel ", target);
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
			Client* client = server->findClientByName(target);
			if (client == nullptr) {
				sendLine("401 ", nick, " ", target, " :No such nick/channel");
				log::warn("PRIVMSG: No such nick: ", target);
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

/**
 * Have the client change the modes for a channel.
 */
void Client::setChannelMode(Channel& channel, char* mode, char* args)
{
	// String containing the set of mode changes that were applied.
	std::string modeOut;
	std::string argsOut;
	char lastSign = 0;

	// Special case to keep irssi happy: Handle 'b' by sending an empty ban list
	// for the channel.
	if (std::strcmp(mode, "b") == 0)
		return sendLine("368 ", nick, " ", channel.name, " :End of channel ban list");

	// Parse the mode string.
	char sign = 0;
	while (*mode != '\0') {

		// Expect either a '+' or a '-'.
		sign = *mode++;
		if (sign != '+' && sign != '-')
			return sendLine("472 ", nick, " ", sign, " :is unknown mode char to me");
		if (!std::isalpha(*mode))
			return sendLine("472 ", nick, " ", *mode, " :is unknown mode char to me");

		// Iterate over the characters in the mode string.
		for (; std::isalpha(*mode); mode++) {
			switch (*mode) {

				// +i: Toggle invite-only mode.
				case 'i': {
					if (channel.inviteOnly == (sign == '+'))
						continue;
					channel.inviteOnly = sign == '+';
					channel.resetInvited();
				} break;

				// +t: Toggle restrict topic mode.
				case 't': {
					if (channel.restrictTopic == (sign == '+'))
						continue;
					channel.restrictTopic = sign == '+';
				} break;

				// +k: Set or remove the channel key.
				case 'k': {
					if (sign == '+') {
						char* key = nextListItem(args);
						if (key == channel.key)
							continue;
						if (!channel.setKey(key)) {
							sendLine("525 ", nick, " ", channel.name, " :Key is not well-formed");
							continue;
						}
						argsOut += " " + std::string(key);
					} else {
						channel.removeKey();
					}
				} break;

				// +l: Set or remove the channel member limit.
				case 'l': {
					if (sign == '+') {
						int limit;
						if (!parseInt(nextListItem(args), limit) || limit <= 0) {
							sendLine("696 ", nick, " ", channel.name, " l ", limit, " :Bad limit");
							continue;
						} else if (limit == channel.memberLimit) {
							continue;
						}
						channel.setMemberLimit(limit);
						argsOut += " " + std::to_string(limit);
					} else {
						if (channel.memberLimit == 0)
							continue;
						channel.removeMemberLimit();
					}
				} break;

				// +o: Give or take operator privileges from a client.
				case 'o': {
					char* target = nextListItem(args);
					Client* client = server->findClientByName(target);
					if (client == nullptr) {
						sendLine("401 ", nick, " ", target, " :No such nick/channel");
						continue;
					}

					// Skip if the mode wouldn't change.
					if ((sign == '+') == channel.isOperator(*client))
						continue;
					if (sign == '+')
						channel.addOperator(*client);
					if (sign == '-')
						channel.removeOperator(*client);
					argsOut += " " + std::string(target);
				} break;

				// Anything else is unrecognized.
				default: {
					 sendLine("502 ", nick, " :Unknown MODE flag");
				} continue;
			}

			// Update the set of added and removed modes.
			if (modeOut.empty() || lastSign != sign) {
				modeOut.push_back(sign);
				lastSign = sign;
			}
			modeOut += *mode;
		}
	}

	// Broadcast a message to all channel members containing only the modes that
	// were actually applied.
	for (Client* member: channel.members)
		member->sendLine("MODE ", channel.name, " ", modeOut, argsOut);
}

/**
 * Handle a MODE message.
 */
void Client::handleMode(int argc, char** argv)
{
	// Check that enough parameters were provided.
	if (argc < 1 || argc > 3)
		return sendLine("461 ", nick, " MODE :Not enough parameters");

	// Check if the target is a channel.
	char* target = argv[0];
	if (Channel::isValidName(target)) {

		// Check that the channel actually exists.
		Channel* channel = server->findChannelByName(target);
		if (channel == nullptr)
			return sendLine("403 ", nick, " ", target, " :No such channel");

		// If no mode string was given, reply with the channel's current modes.
		if (argc < 2)
			return sendLine("324 ", nick, " ", target, " :", channel->getModes());

		// Check that the client has channel operator privileges.
		if (!channel->isOperator(*this))
			return sendLine("482 ", nick, " ", target, " :You're not channel operator");

		// Parse the mode string.
		char noArguments[1] = "";
		char* args = argc < 3 ? noArguments : argv[2];
		setChannelMode(*channel, argv[1], args);

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
			return sendLine("221 ", nick, " :"); // No user modes implemented.

		// User modes are not implemented, but we ignore the +i mode just to
		// keep irssi happy.
		char* mode = argv[1];
		while (*mode) {
			mode += *mode == '+' || *mode == '-';
			if (!std::isalpha(*mode))
				return sendLine("472 ", nick, " ", *mode, " :is unknown mode char to me");
			for (; std::isalpha(*mode); mode++) {
				if (*mode++ != 'i')
					sendLine("502 ", nick, " :Unknown MODE flag");
			}
		}
	}
}

/**
 * Handle a WHO message.
 */
void Client::handleWho(int argc, char** argv)
{
	// Check that the correct number of parameters were given.
	if (argc > 2)
		return sendLine("461 ", nick, " WHO :Not enough parameters");

	// If the server doesn't support the WHO command with a <mask> parameter, it
	// can send just an empty list.
	if (argc == 0) {

		// Send information about each client connected to the server.
		for (auto& [_, client]: server->allClients()) {

			// Get the name of one channel that this client is on, or '*' if the
			// client is not joined to any channels.
			std::string_view channel = "*";
			if (!client.channels.empty())
				channel = (*client.channels.begin())->name;

			// Send some information about the client.
			send("351 ", nick, " ");
			send(channel, " ");
			send(client.user, " ");
			send(client.host, " ");
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
 * Handle a TOPIC command.
 */
void Client::handleTopic(int argc, char** argv)
{
	if (argc < 1 || argc > 2)
		return sendLine("461 ", nick, " NICK :Not enough parameters");

	Channel* channel = server->findChannelByName(argv[0]);
	if (!channel)
		return sendLine("403 ", argv[0], " :No such channel");

	if (argc == 1)
	{
		log::info("Sent topic: ", channel->topic);
		if (!channel->findClientByName(nick))
			return sendLine("442 ", channel->name, " :You're not on that channel");

		if (channel->topic.empty())
			return sendLine("331 ", channel->name, " :No topic is set");

		sendLine("332 ", nick, " ", channel->name, " :", channel->topic);
		sendLine("333 ", nick, " ", channel->name, " ", channel->topicChangeStr);
		return;
	}

	assert(argc == 2);

	if (!channel->isMember(*this))
		return sendLine("442 ", channel->name, " :You're not on that channel");

	// Check that the client has permissions to change the topic.
	if (channel->restrictTopic && !channel->isOperator(*this))
		return sendLine("482 ", nick, " ", channel->name, " :You're not channel operator"); // Grammar mistake according to spec :)

	// Change the topic.
	channel->topic = argv[1];
	channel->topicChangeStr = std::string(nick).append(" ").append(Server::getTimeString());
	log::info("Changed topic of ", channel->name, " to: ", channel->topic);

	// Notify all channel members (including the sender) of the change.
	for (Client* member: channel->members)
		member->sendLine("TOPIC ", channel->name, " :", channel->topic);
}

//forced removal of a user from a channel.
//:PUPU!p@localhost KICK #channel eve :being too rigorous
//:<kicker>!<user>@localhost KICK <channel> <targetToKick> :<reason>
void Client::handleKick(int argc, char** argv)
{
    // Must have at least <channel> and <user>
    if (argc < 2) {
        sendLine("461 ", nick, " KICK :Not enough parameters");
        return;
    }

    std::string channelName = argv[0];
    std::string targetToKick = argv[1];

    // build reason string coz it can be many args
    std::string reason;
    if (argc >= 3) {
		for (int i = 2; i < argc; ++i) {
			reason += (i > 2 ? " " : ""); // add a space before everything except the first word
			reason += argv[i];
		}
        if (!reason.empty() && reason[0] == ':')
            reason.erase(0, 1);
    } else {
        reason = "No reason. I just kicked you for fun";
    }

    // find channel
    Channel* channel = server->findChannelByName(channelName);
    if (!channel) {
        sendLine("403 ", nick, " ", channelName, " :No such channel");
        log::warn("KICK: No such channel: ", channelName);
        return;
    }

    // check sender is in channel
    if (!channel->findClientByName(nick)) {
        sendLine("442 ", nick, " ", channelName, " :You're not on that channel");
        log::warn("KICK: ", nick, " tried to kick but is not a member of ", channelName);
        return;
    }

	// Check that the sender has operator privileges on the channel.
	if (!channel->isOperator(*this))
		return sendLine("482 ", nick, " ", channelName, " :You're not channel operator");

	//if no target found in the channel
    Client* clientToKick = channel->findClientByName(targetToKick);
    if (clientToKick == nullptr) {
        sendLine("441 ", nick, " ", targetToKick, " ", channelName, " :They aren't on that channel");
        log::warn("KICK: ", nick, " tried to kick ", targetToKick, " but they are not in ", channelName);
        return;
    }

    // broadcast kick message
    for (Client* member: channel->members) {
		member->send(":", fullname, " ");
        member->send("KICK ", channelName, " ", targetToKick);
		member->sendLine(" :", reason);
	}

    // remove kicked dude
    channel->removeMember(*clientToKick);

    log::info("KICK: ", nick, " kicked ", targetToKick, " from ", channelName);
}


void Client::handleInvite(int argc, char** argv)
{
	if (argc != 2)
		return sendLine("461 ", nick, " INVITE :Not enough parameters");

	const std::string_view invitedName = argv[0];

	Client* invitedClient = server->findClientByName(invitedName);

	if (!invitedClient)
		return sendLine("406 ", invitedName, " :There was no such nickname");

	Channel* channel = server->findChannelByName(argv[1]);

	if (!channel)
		return sendLine("403 ", invitedName, " :No such channel");

	if (!channel->findClientByName(nick))
		return sendLine("442 ", nick, " ", channel->name, " :You're not on that channel");

	if (channel->findClientByName(invitedName))
		return sendLine("443 ", nick, " ", invitedName, " ", channel->name, " :is already on channel");

	if (!channel->isOperator(*this))
		return sendLine("482 ", nick, " ", channel->name, " :You're not channel operator");

	channel->addInvited(invitedName);
	sendLine("341 ", channel->name, " ", nick);
	invitedClient->sendLine("INVITE ", invitedName, " ", channel->name);
}
