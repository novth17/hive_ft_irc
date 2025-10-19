#include <csignal>
#include <cstring>
#include <netdb.h>
#include <sys/epoll.h>

#include "channel.hpp"
#include "client.hpp"
#include "irc.hpp"
#include "server.hpp"
#include "utility.hpp"

Server::Server(const char* port, const char* password)
	: _port(port), _password(password)
{
	logInfo("Starting server with password ", _password);
}

Server::~Server()
{
	logInfo("Closing connection");
	for (const auto& [fd, client]: _clients)
		close(fd);
	safeClose(_serverFd);
	safeClose(_epollFd);
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
	_serverFd = createListenSocket(host, port, true);
	if (_serverFd == -1)
		fail("Failed to create server socket: ", strerror(errno));

	// Create epoll instance for serverFD.
	_epollFd = epoll_create1(0);
	if (_epollFd == -1)
		fail("Failed to create epoll instance: ", strerror(errno));

	// Add server fd socket to epoll.
	epollEvent.events = EPOLLIN;
	epollEvent.data.fd = _serverFd;
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, _serverFd, &epollEvent) == -1)
		fail("Failed to add server socket to epoll: ", strerror(errno));

	// Begin the event loop.
	logInfo("Listening on port ", _port);
	while (true) {

		// Poll available events.
		int numberOfReadyEvents = epoll_wait(_epollFd, events, MAX_EVENTS, -1);
		if (numberOfReadyEvents == -1) {
			if (errno == EINTR) {
				fprintf(stderr, "\r"); // Just to avoid printing ^C.
				logInfo("Interrupted by user");
				break;
			}
			fail("Failed to wait for events: ", strerror(errno));
		}

		// Loop over pending events.
		for (int i = 0; i < numberOfReadyEvents; ++i) {
			int fd = events[i].data.fd;

			// Accept a new client.
			if (fd == _serverFd) {

				// Open a new client connection.
				sockaddr_in clientAddress;
				socklen_t clientAddressLength = sizeof(clientAddress);
				int clientFd = accept(_serverFd, (struct sockaddr*)&clientAddress, &clientAddressLength);
				if (clientFd == -1)
					fail("Failed to accept connection: ", strerror(errno));
				setNonBlocking(clientFd);

				// Register the connection with epoll.
				Client& client = _clients[clientFd];
				client.socket = clientFd;
				epollEvent.events = EPOLLIN | EPOLLOUT | EPOLLET;
				epollEvent.data.fd = clientFd;
				if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, clientFd, &epollEvent) == -1)
					fail("Failed to add client socket to epoll: ", strerror(errno));
				logInfo("Client connected (fd = ", clientFd, ")");

			// Exchange data with a client.
			} else {

				// Find the Client object for this connection.
				auto found = _clients.find(fd);
				if (found == _clients.end())
					fail("Client for fd ", fd, " not found");
				Client& client = found->second;

				// Exchange data with the client.
				client.send();
				receiveFromClient(client);
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
		bytes = recv(client.socket, buffer, sizeof(buffer), 0);

		// Handle errors.
		if (bytes == -1) {
			if (errno == EAGAIN)
				break; // Nothing more to read.
			fail("Failed to receive from client: ", strerror(errno));

		// Handle client disconnection.
		} else if (bytes == 0) {
			logInfo("Client disconnected (fd = ", client.socket, ")");
			epoll_ctl(_epollFd, EPOLL_CTL_DEL, client.socket, nullptr);
			safeClose(client.socket);
			_clients.erase(client.socket);

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
				logWarn("Message has too many parts:", message);
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
 * Find a specific client by their nickname. Returns a null pointer if there's
 * no client by that nickname.
 */
Client* Server::findClientByName(std::string_view name)
{
	for (auto& [fd, client]: _clients)
		if (client.nick == name)
			return &client;
	return nullptr;
}

/**
 * Find a specific channel by its name. Returns a null pointer if there's no
 * channel by that name.
 */
Channel* Server::findChannelByName(std::string_view name)
{
	for (auto& [channelName, channel]: _channels)
		if (channel.name == name)
			return &channel;
	return nullptr;
}
