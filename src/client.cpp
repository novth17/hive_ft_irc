#include <sys/socket.h>
#include <cstring>

#include "client.hpp"
#include "utility.hpp"
#include "irc.hpp"

/**
 * Create a new Client.
 */
Client::Client(Server& server, int socket, std::string_view host)
	: server(server),
	  socket(socket),
	  host(host)
{
}

/**
 * Get the client's socket file descriptor.
 */
int Client::getSocket() const
{
	return socket;
}

/**
 * Get an iterator pair over the channels the client is joined to.
 */
ClientChannelIterators Client::allChannels()
{
	return {channels.begin(), channels.end()};
}

/**
 * Clear the client's list of channels.
 */
void Client::clearChannels()
{
	channels.clear();
}

/**
 * Get the client's hostname.
 */
std::string_view Client::getHost() const
{
	return host;
}

/**
 * Get the client's long-form name (in the form nick!user@host).
 */
std::string_view Client::getFullName() const
{
	return fullname;
}

/**
 * Get the client's current nickname.
 */
std::string_view Client::getNick() const
{
	return nick;
}

/**
 * Check if the client has been marked as disconnected.
 */
bool Client::isDisconnected() const
{
	return disconnected;
}

/**
 * Mark the client as disconnected.
 */
void Client::setDisconnected()
{
	disconnected = true;
}

void Client::receive()
{
	// Receive data from the client.
	char buffer[512];
	ssize_t bytes = 1;
	while (bytes > 0) {
		bytes = recv(socket, buffer, sizeof(buffer), MSG_DONTWAIT);

		// Handle errors.
		if (bytes == -1) {
			if (errno == EAGAIN || errno == ECONNRESET)
				break; // Nothing more to read.
			fail("Failed to receive from client: ", strerror(errno));

		// Handle client disconnection.
		} else if (bytes == 0) {
			server.disconnectClient(*this);

		// Buffer received data.
		} else {
			input.append(buffer, bytes);

			// Check for complete messages.
			while (true) {
				size_t newline = input.find("\r\n");
				if (newline == input.npos)
					break;
				auto begin = input.begin();
				auto end = input.begin() + newline;
				parseMessage(std::string(begin, end));
				input.erase(0, newline + 2);
			}
		}
	}
}

/**
 * Parse a raw client message, then pass it to the message handler for the
 * message's command, along with any parameters.
 */
void Client::parseMessage(std::string message)
{
	// Array for holding the individual parts of the message.
	int argc = 0;

	// log::info(">>>>>>>>> RECV '", message, "'");
	char* argv[MAX_MESSAGE_PARTS];

	// Split the message into parts.
	size_t begin = 0;
	while (begin < message.length()) {

		// Find the beginning of the next part.
		begin = message.find_first_not_of(' ', begin);
		if (begin == message.npos)
			break; // Reached the end of the message.

		// Find the end of the part (either the next space or the message end).
		size_t end = message.find(' ', begin);
		if (end == message.npos)
			end = message.length();

		// Ignore tags (parts starting with an '@' sign).
		if (message[begin] != '@') {

			// Issue a warning if there are too many parts.
			if (argc == MAX_MESSAGE_PARTS) {
				log::warn("Message has too many parts:", message);
				return;
			}

			// If the part starts with a ':', treat the rest of the message as
			// one big part.
			if (message[begin] == ':') {
				end = message.length();
				begin++; // Strip the ':' from the message.
			}

			// Add the part to the array and null-terminate it.
			argv[argc++] = message.data() + begin;
			message[end] = '\0';
		}

		// Begin the next part at the end of this one.
		begin = end + (end < message.length());
	}

	// Pass the message to its handler.
	handleMessage(argc, argv);
}

/**
 * Handle any type of message. Removes the command from the parameter list, then
 * calls the handler for that command with the remaining parameters.
 */
void Client::handleMessage(int argc, char** argv)
{
	// Ignore empty messages.
	if (argc == 0)
		return;

	// Array of message handlers.
	using Handler = void (Client::*)(int, char**);
	static const std::pair<const char*, Handler> handlers[] = {
		{"USER", &Client::handleUser},
		{"NICK", &Client::handleNick},
		{"PASS", &Client::handlePass},
		{"PART", &Client::handlePart},
		{"JOIN", &Client::handleJoin},
		{"PING", &Client::handlePing},
		{"QUIT", &Client::handleQuit},
		{"MODE", &Client::handleMode},
		{"WHO", &Client::handleWho},
		{"KICK", &Client::handleKick},
		{"PRIVMSG", &Client::handlePrivMsg},
		{"TOPIC",  &Client::handleTopic},
		{"INVITE", &Client::handleInvite},
		{"NAMES", &Client::handleNames},
		{"LIST", &Client::handleList},
		{"LUSERS", &Client::handleLusers},
		{"MOTD", &Client::handleMotd},
		{"NOTICE", &Client::handleNotice},
	};

	// Send the message to the handler for that command.
	for (const auto& [command, handler]: handlers) {
		if (matchIgnoreCase(command, argv[0])) {
			return (this->*handler)(argc - 1, argv + 1);
		}
	}

	// Log any unimplemented commands, so that they can be added eventually.
	// For any other command, send an unknown command error.
	sendNumeric("421", argv[0], " :Unknown command");
	log::warn("Unimplemented command: ", argv[0]);
}

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
	sendNumeric("003", ":This server was created ", server.getLaunchTime());
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
 * that the calling command handler should return immediately.
 *
 * @param cmd  The name of the command (e.g. "JOIN")
 * @param reg  Whether registration is required to use the command
 * @param argc The actual number of parameters
 * @param min  The minimum number of parameters
 * @param max  The maximum number of parameters
 */
bool Client::checkParams(const char* cmd, bool reg, int argc, int min, int max)
{
	// Check registration.
	if (reg && !isRegistered) {
		log::warn("In command ", cmd, ": Client is not registered");
		sendNumeric("451", ":You have not registered");
		return false;
	}

	// Check parameter count.
	if (argc < min || argc > max) {
		log::warn("In command ", cmd, ": Got ", argc, " parameters, expected ", min, "-", max);
		sendNumeric("461", cmd, " :Not enough parameters");
		return false;
	}
	return true;
}
