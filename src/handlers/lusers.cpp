#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "server.hpp"
#include "irc.hpp"

void Client::handleLusers(int argc, char** argv)
{
	(void) argv;

	// Check that no parameters were given.
	if (argc != 0) {
		log::warn(nick, "LUSERS: Has to be 0 params");
		return sendLine("461 ", nick, " LUSERS :Not enough parameters");
	}

	// Send stats about the number of clients and channels. The server doesn's
	// support invisible users or networks, so those values are hard-coded.
	size_t users = server->getClientCount();
	size_t chans = server->getChannelCount();
	sendLine("251 ", fullname, " :There are ", users ," users and 0 invisible on 1 servers");
	sendLine("254 ", fullname, " ", chans, " :channels formed");
	sendLine("255 ", fullname, " :I have ", users, " clients and 1 servers");
}
