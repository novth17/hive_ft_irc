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
		log::warn(nick, " LUSERS: Has to be 0 params");
		return sendNumeric("461", "LUSERS :Not enough parameters");
	}

	// Send stats about the number of clients and channels. The server doesn's
	// support invisible users or networks, so those values are hard-coded.
	size_t users = server->getClientCount();
	size_t chans = server->getChannelCount();
	sendNumeric("251", ":There are ", users ," users and 0 invisible on 1 servers");
	sendNumeric("254", chans, " :channels formed");
	sendNumeric("255", ":I have ", users, " clients and 1 servers");
}
