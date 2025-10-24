#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "server.hpp"
#include "irc.hpp"
#include <cstring>

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
