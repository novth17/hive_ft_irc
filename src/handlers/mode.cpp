#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "server.hpp"
#include "irc.hpp"
#include <cstring>

/**
 * Have the client change the modes for a channel.
 */
void Client::setChannelMode(Channel& channel, char* mode, char* args)
{
	// String containing the set of mode changes that were applied.
	std::string modeOut;
	std::string argsOut;
	char lastSign = 0;

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

	// Broadcast a message to all other channel members containing only the
	// modes that were actually applied.
	for (Client* member: channel.members)
		member->sendLine(":", fullname, " MODE ", channel.name, " ", modeOut, argsOut);
}

/**
 * Handle a MODE message.
 */
void Client::handleMode(int argc, char** argv)
{
	// Check that enough parameters were provided.
	if (argc < 1 || argc > 3) {
		sendLine("461 ", nick, " MODE :Not enough parameters");
		return log::warn(nick, " MODE: Need more params or too many params");
	}
	// Check if the target is a channel.
	char* target = argv[0];
	if (Channel::isValidName(target)) {

		// Check that the channel actually exists.
		Channel* channel = server->findChannelByName(target);
		if (channel == nullptr) {
			sendLine("403 ", nick, " ", target, " :No such channel");
       		return log::warn(" MODE: No such channel: ", target);
		}
		// If no mode string was given, reply with the channel's current modes.
		if (argc < 2) {
			sendLine("324 ", nick, " ", target, " :", channel->getModes());
			return log::info(" MODE: channel current mode: ", channel->getModes());
		}
		// Special case to keep irssi happy: Handle 'b' by sending an empty ban
		// list for the channel.
		if (std::strcmp(argv[1], "b") == 0) {
			sendLine("368 ", nick, " ", target, " :End of channel ban list");
			return log::info(" MODE: End of channel ban list");
		}
		// Check that the client has channel operator privileges.
		if (!channel->isOperator(*this)) {
			sendLine("482 ", nick, " ", target, " :You're not channel operator");
			return log::warn(" MODE: You don't have channel operator privileges");
		}
		// Parse the mode string.
		char noArguments[1] = "";
		char* args = argc < 3 ? noArguments : argv[2];
		setChannelMode(*channel, argv[1], args);

	// Otherwise, the target must be a client.
	} else {

		// Check that the client exists.
		Client* client = server->findClientByName(target);
		if (client == nullptr) {
			sendLine("401 ", nick, " ", target, " :No such nick/channel");
			return log::warn(" MODE: No client can be found for the supplied nickname");
		}
		// Check that the target matches the client's own nickname.
		if (client->nick != target) {
			sendLine("502 ", nick, " :Cant change mode for other users");
			return log::warn(" MODE: Users don't match. Can't view/change modes for other users");
		}
		// If no mode string was given, reply with the client's current modes.
		if (argc < 2) {
			sendLine("221 ", nick, " :"); // No user modes implemented.
			return log::warn(" MODE: No user modes implemented");
		}
		// User modes are not implemented, but we ignore the +i mode just to
		// keep irssi happy.
		char* mode = argv[1];
		while (*mode) {
			mode += *mode == '+' || *mode == '-';
			if (!std::isalpha(*mode)) {
				sendLine("472 ", nick, " ", *mode, " :is unknown mode char to me");
				return log::warn(" MODE: Can't regcognize mode character used by client");
			}
			for (; std::isalpha(*mode); mode++) {
				if (*mode != 'i')
					sendLine("501 ", nick, " :Unknown MODE flag");
			}
		}
	}
}
