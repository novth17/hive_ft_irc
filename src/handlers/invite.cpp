#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "server.hpp"
#include "irc.hpp"
#include <cstring>

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
