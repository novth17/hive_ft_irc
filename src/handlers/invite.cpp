#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "server.hpp"
#include "irc.hpp"
#include <cstring>

/**
 * Handle an INVITE message.
 */
void Client::handleInvite(int argc, char** argv)
{
	if (argc != 2) {
		log::warn(nick, " INVITE: Has to be 2 params: <nickname> <channel>");
		return sendNumeric("461", "INVITE :Not enough parameters");
	}

	// Check that the invited client exists.
	const std::string_view invitedName = argv[0];
	Client* invitedClient = server->findClientByName(invitedName);
	if (invitedClient == nullptr) {
		log::warn("INVITE: No such nickname: ", invitedName);
		return sendNumeric("406", invitedName, " :There was no such nickname");
	}

	// Check that the target channel exists.
	const std::string_view channelName = argv[1];
	Channel* channel = server->findChannelByName(channelName);
	if (channel == nullptr) {
		log::warn("INVITE: No such channel: ", channelName);
		return sendNumeric("403", invitedName, " :No such channel");
	}

	// Check that the sender is actually a member of the channel.
	if (!channel->isMember(*this)) {
		log::warn("INVITE: ", nick, " is not on ", channelName);
		return sendNumeric("442", channelName, " :You're not on that channel");
	}

	// Check that the invited client isn't a member of the channel.
	if (channel->isMember(*invitedClient)) {
		log::warn("INVITE: ", nick, " is already on ", channelName);
		return sendNumeric("443", invitedName, " ", channelName, " :is already on channel");
	}

	// Check that the sender is an operator on the channel.
	if (!channel->isOperator(*this)) {
        log::warn("INVITE: ", nick, " tried to invite but is not a operator of ", channelName);
		return sendNumeric("482", channelName, " :You're not channel operator");
	}

	// Add the invited client to the invite list and notify them.
	channel->addInvited(*invitedClient);
	sendNumeric("341", invitedName, " ", channelName);
	invitedClient->sendLine(":", fullname, " INVITE ", invitedName, " ", channelName);
	log::info(nick, " invited ", invitedName, " to ", channelName);
}
