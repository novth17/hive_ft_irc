#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <netdb.h>
#include <sys/epoll.h>
#include <iomanip>

#include "channel.hpp"
#include "client.hpp"
#include "irc.hpp"
#include "log.hpp"
#include "server.hpp"
#include "utility.hpp"

Server::Server(const char* port, const char* password)
	: port(port), password(password)
{
	launchTime = getTimeString();
	log::info("Starting server with password ", password);
}

Server::~Server()
{
	log::info("Closing connection");
	for (auto& [fd, client]: clients) {
		client.sendLine("ERROR :Server is shutting down");
		close(fd);
	}
	safeClose(serverFd);
	safeClose(epollFd);
}

void Server::eventLoop(const char* host, const char* port)
{
	 // Ignore SIGINT.
	struct sigaction sa = {};
	sa.sa_handler = [] (int) {}; // Do-nothing function.
	sigaction(SIGINT, &sa, nullptr);

	// Create epoll.
	struct epoll_event epollEvent, events[MAX_EVENTS];

	// Create listen socket.
	serverFd = createListenSocket(host, port);
	if (serverFd == -1)
		fail("Failed to create server socket: ", strerror(errno));

	// Create epoll instance for serverFD.
	epollFd = epoll_create1(0);
	if (epollFd == -1)
		fail("Failed to create epoll instance: ", strerror(errno));

	// Add server fd socket to epoll.
	epollEvent.events = EPOLLIN;
	epollEvent.data.fd = serverFd;
	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverFd, &epollEvent) == -1)
		fail("Failed to add server socket to epoll: ", strerror(errno));

	// Begin the event loop.
	log::info("Listening on port ", port);
	while (true) {

		// Poll available events.
		int numberOfReadyEvents = epoll_wait(epollFd, events, MAX_EVENTS, -1);
		if (numberOfReadyEvents == -1) {
			if (errno == EINTR) {
				fprintf(stderr, "\r"); // Just to avoid printing ^C.
				log::info("Interrupted by user");
				break;
			}
			fail("Failed to wait for events: ", strerror(errno));
		}

		// Loop over pending events.
		for (int i = 0; i < numberOfReadyEvents; ++i) {
			int fd = events[i].data.fd;

			// Accept a new client.
			if (fd == serverFd) {

				// Open a new client connection.
				struct sockaddr_in address;
				struct sockaddr* sockaddr = reinterpret_cast<struct sockaddr*>(&address);
				socklen_t length = sizeof(address);
				int clientFd = accept(serverFd, sockaddr, &length);
				if (clientFd == -1)
					fail("Failed to accept connection: ", strerror(errno));

				// Register the connection with epoll.
				Client& client = clients[clientFd];
				client.server = this;
				client.socket = clientFd;
				client.host = inet_ntoa(address.sin_addr);
				epollEvent.events = EPOLLIN | EPOLLOUT | EPOLLET;
				epollEvent.data.fd = clientFd;
				if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &epollEvent) == -1)
					fail("Failed to add client socket to epoll: ", strerror(errno));
				log::info("Client connected: ", client.host);

			// Exchange data with a client.
			} else {

				// Find the Client object for this connection.
				auto found = clients.find(fd);
				if (found == clients.end())
					fail("Client for fd ", fd, " not found");
				Client& client = found->second;

				// Exchange data with the client.
				client.send();
				receiveFromClient(client);
			}
		}

		// Remove any clients that were disconnected during the last iteration
		// of the event loop. It's important not to do this in the middle of the
		// send/receive part of the event loop, when the socket is still
		// actively used.
		for (auto i = clients.begin(); i != clients.end();) {
			if (i->second.isDisconnected) {
				close(i->first);
				log::info("Client disconnected: ", i->second.host);
				i = clients.erase(i);
			} else {
				++i;
			}
		}
	}
}

void Server::receiveFromClient(Client& client)
{
	// Receive data from the client.
	char buffer[512];
	ssize_t bytes = 1;
	while (bytes > 0) {
		bytes = recv(client.socket, buffer, sizeof(buffer), MSG_DONTWAIT);

		// Handle errors.
		if (bytes == -1) {
			if (errno == EAGAIN || errno == ECONNRESET)
				break; // Nothing more to read.
			fail("Failed to receive from client: ", strerror(errno));

		// Handle client disconnection.
		} else if (bytes == 0) {
			client.isDisconnected = true;

		// Buffer received data.
		} else {
			std::string& input = client.input.append(buffer, bytes);

			// Check for complete messages.
			while (true) {
				size_t newline = input.find("\r\n");
				if (newline == input.npos)
					break;
				auto begin = input.begin();
				auto end = input.begin() + newline;
				parseMessage(client, std::string(begin, end));
				input.erase(0, newline + 2);
			}
		}
	}
}

/**
 * Parse a raw client message, then pass it to the message handler for the
 * message's command, along with any parameters.
 */
void Server::parseMessage(Client& client, std::string message)
{
	// Array for holding the individual parts of the message.
	int argc = 0;
	// log::info(">>> Message: '", message, "'");
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
	handleMessage(client, argc, argv);
}

/**
 * Handle any type of message. Removes the command from the parameter list, then
 * calls the handler for that command with the remaining parameters.
 */
void Server::handleMessage(Client& client, int argc, char** argv)
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
		{"WHO",  &Client::handleWho},
		{"KICK", &Client::handleKick},
		{"PRIVMSG",  &Client::handlePrivMsg},
		{"TOPIC",  &Client::handleTopic},
		{"INVITE",  &Client::handleInvite}
	};

	// Send the message to the handler for that command.
	for (const auto& [command, handler]: handlers) {
		if (matchIgnoreCase(command, argv[0])) {
			return (client.*handler)(argc - 1, argv + 1);
		}
	}

	// Log any unimplemented commands, so that they can be added eventually.
	log::error("Unimplemented command: ", argv[0]);
}

/**
 * Check if a string matches the server password.
 */
bool Server::correctPassword(std::string_view pass)
{
	return password == pass;
}

/**
 * For any two clients, check if those clients share at least one channel.
 */
bool Server::clientsOnSameChannel(const Client& a, const Client& b)
{
	for (Channel* channel: a.channels)
		if (b.channels.find(channel) != b.channels.end())
			return true;
	return false;
}

/**
 * Disconnect a client from the server. Removes the client from all channels
 * they're part of, and also sends a QUIT message to notify others that the
 * client disappeared.
 */
void Server::disconnectClient(Client& client, std::string_view reason)
{
	// Send an ERROR message to the disconnected client, with the reason for the
	// disconnection.
	client.sendLine("ERROR :", reason);

	// Send QUIT messages to let other clients know the client disconnected.
	// The <source> of the message is the disconnected client.
	for (auto& [_, other]: clients)
		if (clientsOnSameChannel(client, other))
			other.sendLine(":", client.nick, " QUIT :", reason);

	// Remove the client from all its channels.
	for (Channel* channel: client.channels)
		channel->removeMember(client);
	client.channels.clear();

	// Unsubscribe from epoll events for the client connection.
	int socket = client.socket;
	epoll_ctl(epollFd, EPOLL_CTL_DEL, socket, nullptr);

	// Mark the client as disconnected. The connection is actually closed before
	// the next iteration of the event loop.
	client.isDisconnected = true;
}

/**
 * Find a specific channel by its name. Returns a null pointer if there's no
 * channel by that name.
 */
Channel* Server::findChannelByName(std::string_view name)
{
	for (auto& [channelName, channel]: channels)
		if (channel.name == name)
			return &channel;
	return nullptr;
}

/**
 * Create a new empty channel and set its name.
 */
Channel* Server::newChannel(const std::string& name)
{
	log::info("Creating new channel ", name);
	Channel* channel = &channels[name];
	channel->server = this;
	channel->name = name;
	return channel;
}

/**
 * Find a specific client by their nickname. Returns a null pointer if there's
 * no client by that nickname.
 */
Client* Server::findClientByName(std::string_view name)
{
	for (auto& [fd, client]: clients)
		if (client.nick == name)
			return &client;
	return nullptr;
}

std::string Server::getLaunchTime()
{
	return launchTime;
}

std::string Server::getTimeString()
{
	time_t _tm = time(NULL);
	struct tm* curtime = localtime(&_tm);

	std::string timeString = asctime(curtime);
	timeString.erase(timeString.length() - 1, 1); // Removes newline

	return timeString;
}
