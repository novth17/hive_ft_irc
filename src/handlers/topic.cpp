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
	if (argc < 1 || argc > 2) {
		sendLine("461 ", nick, " TOPIC :Not enough parameters");
		return log::warn(nick, "TOPIC: Has to be 1-2 params: <channel> [<topic>]");
	}
	Channel* channel = server->findChannelByName(argv[0]);
	if (!channel) {
		sendLine("403 ", argv[0], " :No such channel");
		return log::warn(nick, "TOPIC: There was no such channel");
	}
	if (argc == 1)
	{
		log::info("Sent topic: ", channel->topic);
		if (!channel->findClientByName(nick)) {
			sendLine("442 ", channel->name, " :You're not on that channel");
			return log::warn(nick, "TOPIC: You're not on that channel");
		}
		if (channel->topic.empty()) {
			sendLine("331 ", channel->name, " :No topic is set");
			return log::warn(nick, "TOPIC: Channel's top ic is empty");
		}
		sendLine("332 ", nick, " ", channel->name, " :", channel->topic);
		sendLine("333 ", nick, " ", channel->name, " ", channel->topicChangeStr);
		return;
	}

	assert(argc == 2);

	if (!channel->isMember(*this)) {
		sendLine("442 ", channel->name, " :You're not on that channel");
		return log::warn(nick, "TOPIC: You're not on that channel");
	}
	// Check that the client has permissions to change the topic.
	if (channel->restrictTopic && !channel->isOperator(*this)) {
		sendLine("482 ", nick, " ", channel->name, " :You're not channel operator"); // Grammar mistake according to spec :)
        return log::warn("TOPIC: ", nick, " tried to change topic but is not a operator of ", channel->name);
	}
	// Change the topic.
	channel->topic = argv[1];
	channel->topicChangeStr = std::string(nick).append(" ").append(Server::getTimeString());
	log::info("Changed topic of ", channel->name, " to: ", channel->topic);

	// Notify all channel members (including the sender) of the change.
	for (Client* member: channel->members)
		member->sendLine("TOPIC ", channel->name, " :", channel->topic);
}
