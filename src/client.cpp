#include <cstring>
#include <string_view>
#include <sys/socket.h>
#include <cstring>

#include "client.hpp"
#include "channel.hpp"
#include "utility.hpp"
#include "server.hpp"
#include "irc.hpp"
#include "log.hpp"

/**
 * Send a string of text to the client. All the other variants of the
 * Client::send method call this one to do their business.
 */
void Client::send(const std::string_view& string)
{
	output.append(string);
	ssize_t bytes = 1;
	while (bytes > 0 && output.find("\r\n") != output.npos) {
		bytes = ::send(socket, output.data(), output.size(), MSG_DONTWAIT);
		if (bytes == -1) {
			if (errno == EAGAIN || errno == ECONNRESET)
				break;
			fail("Failed to send to client: ", strerror(errno));
		}
		output.erase(0, bytes);
	}
}

void Client::handleRegistrationComplete()
{
	if (nick.empty() || user.empty() || !isPassValid)
		return;
	isRegistered = true;
	fullname = nick + "!" + user + "@" + host;
	sendLine("001 ", nick, " :Welcome to the ", SERVER_NAME, " Network ", fullname);
	sendLine("002 ", nick, " :Your host is ", SERVER_NAME, ", running version 1.0");
	sendLine("003 ", nick, " :This server was created ", server->getLaunchTime());
	sendLine("004 ", nick, " ", SERVER_NAME, " Version 1.0");
}

/**
 * Perform common parameter checks for commands, sending any error messages if
 * there are problems. Returns false if any of the checks failed, indicating
 * that the calling command handler should return immediately. The maximum
 * parameter count can be left out, in which case there's no parameter limit.
 *
 * @param cmd  The name of the command (e.g. "JOIN")
 * @param reg  Whether registration is reuired to use the command
 * @param argc The actual number of parameters
 * @param min  The minimum number of parameters
 * @param max  The maximum number of parameters
 */
bool Client::commonChecks(const char* cmd, bool reg, int argc, int min, int max)
{
	// Check registration.
	if (reg && !isRegistered) {
		sendLine("451 ", nick, " :You have not registered");
		return false;
	}

	// Check parameter count.
	if (argc < min || argc > max) {
		sendLine("461 ", nick, " ", cmd, " :Not enough parameters");
		return false;
	}
	return true;
}
