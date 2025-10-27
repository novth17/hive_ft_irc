#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "server.hpp"
#include "irc.hpp"
#include <cstring>

/**
 * Handle a KICK message.
 */
void Client::handleKick(int argc, char** argv)
{
	if (!checkParams("KICK", true, argc, 2, 3))
		return;

	// Find the target channel.
	char* channelName = argv[0];
	Channel* channel = server.findChannelByName(channelName);
	if (channel == nullptr) {
		log::warn("KICK: No such channel: ", channelName);
		return sendNumeric("403", channelName, " :No such channel");
	}

	// Check that the sender is in the channel.
	if (!channel->isMember(*this)) {
		log::warn("KICK: ", nick, " tried to kick but is not a member of ", channelName);
		return sendNumeric("442", channelName, " :You're not on that channel");
	}

	// Check that the sender has operator privileges on the channel.
	if (!channel->isOperator(*this)) {
		log::warn("KICK: ", nick, " tried to kick but is not an operator of ", channelName);
		return sendNumeric("482", channelName, " :You're not channel operator");
	}

	// Use an empty <reason> if none was given.
	std::string reason = argc == 3 ? argv[2] : "";

	// Limit the <reason>'s length based on the KICKLEN setting.
	if (reason.length() > KICKLEN)
		reason.resize(KICKLEN);

	// Process the list of target clients to kick.
	char* targetList = argv[1];
	while (*targetList != '\0') {

		// Find the target channel member.
		char* targetName = nextListItem(targetList);
		Client* target = channel->findClientByName(targetName);
		if (target == nullptr) {
			log::warn("KICK: ", nick, " tried to kick ", targetName, " but they are not in ", channelName);
			return sendNumeric("441", targetName, " ", channelName, " :They aren't on that channel");
		}

		// Broadcast kick message.
		for (Client* member: channel->allMembers()) {
			member->send(":", fullname, " ");
			member->send("KICK ", channelName, " ", targetName);
			member->sendLine(" :", reason);
		}

		// Remove kicked dude.
		channel->removeMember(*target);
		log::info("KICK: ", nick, " kicked ", targetName, " from ", channelName);
	}
}
