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
	const int sendFlags = MSG_DONTWAIT | MSG_NOSIGNAL;
	while (bytes > 0 && output.find("\r\n") != output.npos) {
		bytes = ::send(socket, output.data(), output.size(), sendFlags);
		// log::info(">>>>>>>>> SEND '", std::string_view(output.data(), output.size() - 2), "'");
		if (bytes == -1) {
			if (errno == EAGAIN || errno == ECONNRESET || errno == EPIPE)
				break;
			fail("Failed to send to client: ", strerror(errno));
		}
		output.erase(0, bytes);
	}
}

/**
 * Send the mandatory set of messages when a clint completes registration, by
 * providing the full set of nickname/username/password.
 */
void Client::handleRegistrationComplete()
{
	// Check that all credentials were received.
	if (nick.empty() || user.empty() || !isPassValid)
		return;

	// Update the client's status.
	isRegistered = true;
	fullname = nick + "!" + user + "@" + host;

	// Send welcome messages.
	sendNumeric("001", ":Welcome to the ", SERVER_NAME, " Network ", fullname);
	sendNumeric("002", ":Your host is ", SERVER_NAME, ", running version 1.0");
	sendNumeric("003", ":This server was created ", server->getLaunchTime());
	sendNumeric("004", ":" SERVER_NAME " Version 1.0");

	// Send feature advertisement messages (at least one is mandatory).
	const char* features[] = {
		"CASEMAPPING=ascii",
	};
	for (const char* feature: features)
		sendNumeric("005", feature, " :are supported by this server");

	// Respond as if the LUSERS and MOTD commands had been sent.
	handleLusers(0, nullptr);
	handleMotd(0, nullptr);
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
		sendNumeric("451", ":You have not registered");
		return false;
	}

	// Check parameter count.
	if (argc < min || argc > max) {
		sendNumeric("461", cmd, " :Not enough parameters");
		return false;
	}
	return true;
}
