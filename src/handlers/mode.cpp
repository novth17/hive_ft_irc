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
			return sendNumeric("472", sign, " :is unknown mode char to me");
		if (!std::isalpha(*mode))
			return sendNumeric("472", *mode, " :is unknown mode char to me");

		// Iterate over the characters in the mode string.
		for (; std::isalpha(*mode); mode++) {
			switch (*mode) {

				// +i: Toggle invite-only mode.
				case 'i': {
					if (channel.isInviteOnly() == (sign == '+'))
						continue;
					channel.setInviteOnly(sign == '+');
					channel.resetInvited();
				} break;

				// +t: Toggle restrict topic mode.
				case 't': {
					if (channel.isTopicRestricted() == (sign == '+'))
						continue;
					channel.setTopicRestricted(sign == '+');
				} break;

				// +k: Set or remove the channel key.
				case 'k': {
					if (sign == '+') {
						char* key = nextListItem(args);
						if (*key != '\0' && key == channel.getKey())
							continue;
						if (!channel.setKey(key)) {
							sendNumeric("525", channel.getName(), " :Key is not well-formed");
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
							sendNumeric("696", channel.getName(), " l ", limit, " :Bad limit");
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
						sendNumeric("401", target, " :No such nick/channel");
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
					 sendNumeric("502", ":Unknown MODE flag");
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
		member->sendLine(":", fullname, " MODE ", channel.getName(), " ", modeOut, argsOut);
}

/**
 * Handle a MODE message.
 */
void Client::handleMode(int argc, char** argv)
{
	if (!checkParams("MODE", true, argc, 1, 3))
		return;

	// Check if the target is a channel.
	char* target = argv[0];
	if (Channel::isValidName(target)) {

		// Check that the channel actually exists.
		Channel* channel = server->findChannelByName(target);
		if (channel == nullptr)
			return sendNumeric("403", target, " :No such channel");

		// If no mode string was given, reply with the channel's current modes.
		if (argc < 2)
			return sendNumeric("324", target, " :", channel->getModes());

		// Special case to keep irssi happy: Handle 'b' by sending an empty ban
		// list for the channel.
		if (std::strcmp(argv[1], "b") == 0)
			return sendNumeric("368", target, " :End of channel ban list");

		// Check that the client has channel operator privileges.
		if (!channel->isOperator(*this))
			return sendNumeric("482", target, " :You're not channel operator");

		// Parse the mode string.
		char noArguments[1] = "";
		char* args = argc < 3 ? noArguments : argv[2];
		setChannelMode(*channel, argv[1], args);

	// Otherwise, the target must be a client.
	} else {

		// Check that the client exists.
		Client* client = server->findClientByName(target);
		if (client == nullptr)
			return sendNumeric("401", target, " :No such nick/channel");

		// Check that the target matches the client's own nickname.
		if (client->nick != target)
			return sendNumeric("502", ":Cant change mode for other users");

		// If no mode string was given, reply with the client's current modes.
		if (argc < 2)
			return sendNumeric("221", ":"); // No user modes implemented.

		// User modes are not implemented, but we ignore the +i mode just to
		// keep irssi happy.
		char* mode = argv[1];
		while (*mode) {
			mode += *mode == '+' || *mode == '-';
			if (!std::isalpha(*mode))
				return sendNumeric("472", *mode, " :is unknown mode char to me");
			for (; std::isalpha(*mode); mode++) {
				if (*mode != 'i')
					sendNumeric("502", ":Unknown MODE flag");
			}
		}
	}
}
