#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "server.hpp"
#include "irc.hpp"
#include <cstring>

void Client::handleInvite(int argc, char** argv)
{
	if (argc != 2) {
		sendLine("461 ", nick, " INVITE :Not enough parameters");
		return log::warn(nick, "INVITE: Has to be 2 params: <nickname> <channel>");
	}
	const std::string_view invitedName = argv[0];

	Client* invitedClient = server->findClientByName(invitedName);

	if (!invitedClient) {
		sendLine("406 ", invitedName, " :There was no such nickname");
		return log::warn(nick, "INVITE: There was no such nickname");
	}
	Channel* channel = server->findChannelByName(argv[1]);

	if (!channel) {
		sendLine("403 ", invitedName, " :No such channel");
		return log::warn(nick, "INVITE: There was no such channel");
	}

	if (!channel->findClientByName(nick)) {
		sendLine("442 ", nick, " ", channel->name, " :You're not on that channel");
		return log::warn(nick, "INVITE: You're not on that channel");
	}

	if (channel->findClientByName(invitedName)) {
		sendLine("443 ", nick, " ", invitedName, " ", channel->name, " :is already on channel");
		return log::warn(nick, "INVITE: You're already on this channel");
	}

	if (!channel->isOperator(*this)) {
		sendLine("482 ", nick, " ", channel->name, " :You're not channel operator");
        return log::warn("INVITE: ", nick, " tried to invite but is not a operator of ", channel->name);
	}

	channel->addInvited(invitedName);
	sendLine("341 ", channel->name, " ");
	invitedClient->sendLine("INVITE ", invitedName, " ", channel->name);
	log::info(nick, " invited ", invitedName, " to the channel ", channel->name);
}
