#include <signal.h>

#include "events.hpp"
#include "irc.hpp"

Server::Server(const char* port, const char* password)
	: _port(port), _password(password)
{
	logInfo("Starting server with password '%s'", _password);
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
	_serverFd = SocketUtils::createListenSocket(host, port, true);
	if (_serverFd == -1)
		throwf("Failed to create server socket: %s", strerror(errno));

	// Create epoll instance for serverFD.
	_epollFd = epoll_create1(0);
	if (_epollFd == -1)
		throwf("Failed to create epoll instance: %s", strerror(errno));

	// Add server fd socket to epoll.
	epollEvent.events = EPOLLIN;
	epollEvent.data.fd = _serverFd;
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, _serverFd, &epollEvent) == -1)
		throwf("Failed to add server socket to epoll: %s", strerror(errno));

	// Begin the event loop.
	logInfo("Listening on port %s", _port);
	while (true) {

		// Poll available events.
		int numberOfReadyEvents = epoll_wait(_epollFd, events, MAX_EVENTS, -1);
		if (numberOfReadyEvents == -1) {
			if (errno == EINTR) {
				std::cerr << "\r"; // Just to avoid printing ^C.
				logInfo("Interrupted by user");
				break;
			}
			throwf("Failed to wait for events: %s", strerror(errno));
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
					throwf("Failed to accept connection: %s", strerror(errno));
				SocketUtils::setNonBlocking(clientFd);

				// Register the connection with epoll.
				Client& client = _clients[clientFd];
				client.socket = clientFd;
				epollEvent.events = EPOLLIN;
				epollEvent.data.fd = clientFd;
				if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, clientFd, &epollEvent) == -1)
					throwf("Failed to add client socket to epoll: %s", strerror(errno));
				logInfo("Client connected (fd = %d)", clientFd);

			// Receive data from a client.
			} else {

				// Find the Client object for this connection.
				auto found = _clients.find(fd);
				if (found == _clients.end())
					throwf("Client for fd %d not found", fd);
				Client& client = found->second;

				// Receive data from the socket.
				char buffer[512];
				ssize_t bytesRead = recv(fd, buffer, sizeof(buffer), 0);

				// Handle errors.
				if (bytesRead == -1) {
					throwf("Failed to receive from client: %s", strerror(errno));

				// Handle client disconnection.
				} else if (bytesRead == 0) {
					logInfo("Client disconnected (fd = %d)", fd);
					epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, nullptr);
					_clients.erase(fd);
					safeClose(fd);

				// Buffer received data and check for complete messages.
				} else {
					std::string& input = client.input.append(buffer, bytesRead);
					while (true) {
						size_t newline = input.find("\r\n");
						if (newline == input.npos)
							break;
						input[newline] = '\0';
						parseMessage(input.data());
						input.erase(0, newline + 2);
					}
				}
			}
		}
	}
}

void Server::handleMessage(char** parts, int partCount)
{
	// Remove the command from the parameter array.
	char* command = parts[0];
	partCount--;
	parts++;

	// Send the message to the handler for that command.
	if (matchIgnoreCase(command, "USER"))
		return handleUser(parts, partCount);
	if (matchIgnoreCase(command, "NICK"))
		return handleNick(parts, partCount);
	if (matchIgnoreCase(command, "PASS"))
		return handlePass(parts, partCount);
	if (matchIgnoreCase(command, "CAP"))
		return handleCap(parts, partCount);
	if (matchIgnoreCase(command, "JOIN"))
		return handleJoin(parts, partCount);
	logError("Unimplemented command '%s'", command);
}

void Server::parseMessage(std::string message)
{
	// Array for holding the individual parts of the message.
	int partCount = 0;
	char* parts[MAX_MESSAGE_PARTS];

	// Split the message into parts.
	size_t start = 0;
	while (start < message.length()) {

		// Find the start of the next part.
		start = message.find_first_not_of(' ', start);
		if (start == message.npos)
			break; // Reached the end of the message.

		// Find the end of the part (either the next space or the message end).
		size_t end = message.find(' ', start);
		if (end == message.npos)
			end = message.length();

		// Ignore tags (parts starting with an '@' sign).
		if (message[start] != '@') {

			// Issue a warning if there are too many parts.
			if (partCount == MAX_MESSAGE_PARTS) {
				int length = message.length();
				char* data = message.data();
				logWarn("Message '%.*s' has too many parts", length, data);
				return;
			}

			// If the part starts with a ':', treat the rest of the message as
			// one big part.
			if (message[start] == ':') {
				end = message.length();
				start++; // Strip the ':' from the message.
			}

			// Add the part to the array and null-terminate it.
			parts[partCount++] = message.data() + start;
			message[end] = '\0';
		}

		// Start the next part at the end of this one.
		start = end;
	}

	// Handle the message only if it's not empty.
	if (partCount > 0)
		handleMessage(parts, partCount);
}

void Server::handleUser(char** params, int paramCount)
{
	(void) params, (void) paramCount;
	logWarn("Unimplemented command: JOIN");
}

void Server::handleNick(char** params, int paramCount)
{
	(void) params, (void) paramCount;
	logWarn("Unimplemented command: NICK");
}

void Server::handlePass(char** params, int paramCount)
{
	(void) params, (void) paramCount;
	logWarn("Unimplemented command: PASS");
}

void Server::handleCap(char** params, int paramCount)
{
	(void) params, (void) paramCount;
	logWarn("Unimplemented command: CAP");
}

void Server::handleJoin(char** params, int paramCount)
{
	(void) params, (void) paramCount;
	logWarn("Unimplemented command: JOIN");
}
