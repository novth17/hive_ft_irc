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
	// Check that the correct number of parameters were given.
	if (!checkParams("TOPIC", true, argc, 1, 2))
		return;

	// Check that the channel exists.
	char* channelName = argv[0];
	Channel* channel = server->findChannelByName(channelName);
	if (channel == nullptr) {
		log::warn(nick, " TOPIC: There was no such channel");
		return sendNumeric("403", channelName, " :No such channel");
	}

	// Check that the client is a member of the channel.
	if (!channel->isMember(*this)) {
		log::warn(nick, " TOPIC: You're not on that channel");
		return sendNumeric("442", channel->getName(), " :You're not on that channel");
	}

	// Query the current topic.
	if (argc == 1) {

		// Check if the topic is not set.
		if (channel->hasTopic()) {
			log::warn(nick, " TOPIC: Channel's topic is empty");
			return sendNumeric("331", channel->getName(), " :No topic is set");
		}

		// Reply with the current topic.
		sendNumeric("332", channel->getName(), " :", channel->getTopic());
		sendNumeric("333", channel->getName(), " :", channel->getTopicChange());
		return log::info("Sent topic: ", channel->getTopic());
	}

	// Check that the client has permissions to change the topic.
	if (channel->isTopicRestricted() && !channel->isOperator(*this)) {
        log::warn("TOPIC: ", nick, " tried to change topic but is not a operator of ", channel->getName());
		return sendNumeric("482", channel->getName(), " :You're not channel operator");
	}

	// Change the topic.
	channel->setTopic(argv[1], *this);

	// Notify all channel members (including the sender) of the change.
	for (Client* member: channel->allMembers())
		member->sendLine(":", fullname, " TOPIC ", channel->getName(), " :", channel->getTopic());
}
