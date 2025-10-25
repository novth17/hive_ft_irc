#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "server.hpp"
#include "irc.hpp"
#include <cstring>

//:PUPU!p@localhost KICK #channel eve :being too rigorous
//:<kicker>!<user>@localhost KICK <channel> <targetToKick> :<reason>
void Client::handleKick(int argc, char** argv)
{
    // Must have at least <channel> and <user>
    if (argc < 2) {
        sendLine("461 ", nick, " KICK :Not enough parameters");
		return log::warn(nick, " KICK: Need more params or too many params");
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
	if (!channel->isOperator(*this))  {
		sendLine("482 ", nick, " ", channelName, " :You're not channel operator");
        return log::warn("KICK: ", nick, " tried to kick but is not a operator of ", channelName);
	}
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
