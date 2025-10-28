#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <netdb.h>
#include <sys/epoll.h>

#include "channel.hpp"
#include "client.hpp"
#include "irc.hpp"
#include "log.hpp"
#include "server.hpp"
#include "utility.hpp"

Server::Server(const char* port, const char* password)
	: port(port), password(password)
{
	if (*password == '\0')
		log::info("Starting server with no password");
	else
		log::info("Starting server with password '", password, "'");
	launchTime = getTimeString();
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

/**
 * Create a socket file descriptor for listening for incoming connections.
 */
int Server::createListenSocket(const char* port)
{
	struct addrinfo *ai = nullptr;

	try {
		// Get address info for the listening socket.
		struct addrinfo hints = {};
		hints.ai_family   = AF_INET;		// IPv4 only (not IPv6).
		hints.ai_socktype = SOCK_STREAM;	// TCP only (not UDP).
		hints.ai_flags = AI_PASSIVE;   		// Accept any connections.
		int status = getaddrinfo(nullptr, port, &hints, &ai);
		if (status != 0)
			fail("getaddrinfo() failed: ", gai_strerror(status));

		// Create the listening socket.
		serverFd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (serverFd == -1)
			fail("socket() failed: ", strerror(errno));

		// Allow reuse of the same port in successive runs of the server. Avoids
		// the "address already in use" error when bind() is called.
		int opt = 1;
		if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
			fail("setsockopt() failed:", strerror(errno));

		// Bind the socket.
		if (bind(serverFd, ai->ai_addr, ai->ai_addrlen) == -1)
			fail("bind failed: ", strerror(errno));

		// Start listening for incoming connections.
		if (listen(serverFd, MAX_BACKLOG) == -1)
			fail("listen failed: ", strerror(errno));
		freeaddrinfo(ai);
	
	// Free resources if any of the preceding syscalls failed.
	} catch (...) {
		freeaddrinfo(ai);
		throw; // Rethrow the same exception.
	}
	return serverFd;
}

void Server::eventLoop(const char* port)
{
	// Install a signal handler for SIGINT, so that the server can be shut down
	// gracefully with Ctrl + C.
	static volatile sig_atomic_t caughtSignal;
	struct sigaction sa = {};
	sa.sa_handler = [] (int signal) { caughtSignal = signal; };
	sigaction(SIGINT, &sa, nullptr);

	// Create epoll.
	struct epoll_event epollEvent, events[MAX_EVENTS];

	// Create listen socket.
	serverFd = createListenSocket(port);
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
			if (errno == EINTR && caughtSignal == SIGINT) {
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
				Client& client = newClient(clientFd, inet_ntoa(address.sin_addr));
				epollEvent.events = EPOLLIN | EPOLLOUT | EPOLLET;
				epollEvent.data.fd = clientFd;
				if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &epollEvent) == -1)
					fail("Failed to add client socket to epoll: ", strerror(errno));
				log::info("Client connected: ", client.getHost());

			// Exchange data with a client.
			} else {

				// Find the Client object for this connection.
				auto found = clients.find(fd);
				if (found == clients.end())
					fail("Client for fd ", fd, " not found");
				Client& client = found->second;

				// Exchange data with the client.
				client.send();
				client.receive();
			}
		}

		// Remove any clients that were disconnected during the last iteration
		// of the event loop. It's important not to do this in the middle of the
		// send/receive part of the event loop, when the socket is still
		// actively used.
		for (auto i = clients.begin(); i != clients.end();) {
			if (i->second.isDisconnected()) {
				close(i->first);
				log::info("Client disconnected: ", i->second.getHost());
				i = clients.erase(i);
			} else {
				++i;
			}
		}

		// Clean up channels that were left empty at the end of the last
		// iteration of the event loop.
		for (auto i = channels.begin(); i != channels.end();) {
			if (i->second.isEmpty()) {
				log::info("Removed empty channel ", i->second.getName());
				i = channels.erase(i);
			} else {
				++i;
			}
		}
	}
}

/**
 * Check if a string matches the server password. Also returns true if no
 * password is required for the server.
 */
bool Server::correctPassword(std::string_view pass)
{
	return password.empty() || password == pass;
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
	// The <source> of the message is the disconnected client. Also remove the
	// client from all channels it's a part of.
	for (Channel* channel: client.allChannels()) {
		for (Client* member: channel->allMembers())
			member->sendLine(":", client.getFullName(), " QUIT :", reason);
		channel->removeMember(client);
	}
	client.clearChannels();

	// Unsubscribe from epoll events for the client connection.
	epoll_ctl(epollFd, EPOLL_CTL_DEL, client.getSocket(), nullptr);

	// Mark the client as disconnected. The connection is actually closed before
	// the next iteration of the event loop.
	client.setDisconnected();
}

/**
 * Find a specific channel by its name. Returns a null pointer if there's no
 * channel by that name.
 */
Channel* Server::findChannelByName(std::string_view name)
{
	for (auto& [channelName, channel]: channels)
		if (channel.getName() == name)
			return &channel;
	return nullptr;
}

/**
 * Create a new empty channel and set its name.
 */
Channel* Server::newChannel(const std::string& name)
{
	log::info("Creating new channel ", name);
	return &channels.insert({name, Channel(name)}).first->second;
}

/**
 * Create a new client from a connection file descriptor and a host address.
 */
Client& Server::newClient(int fd, std::string_view host)
{
	return clients.insert({fd, Client(*this, fd, host)}).first->second;
}

/**
 * Find a specific client by their nickname. Returns a null pointer if there's
 * no client by that nickname.
 */
Client* Server::findClientByName(std::string_view name)
{
	for (auto& [fd, client]: clients)
		if (client.getNick() == name)
			return &client;
	return nullptr;
}

/**
 * Get a text timestamp of when the server was started.
 */
std::string Server::getLaunchTime()
{
	return launchTime;
}

/**
 * Get the current time as a string.
 */
std::string Server::getTimeString()
{
	time_t _tm = time(NULL);
	struct tm* curtime = localtime(&_tm);
	std::string timeString = asctime(curtime);
	timeString.pop_back(); // Remove newline.
	return timeString;
}

/**
 * Get the total number of connected clients.
 */
size_t Server::getClientCount() const
{
	return clients.size();
}

/**
 * Get the total number of active channels.
 */
size_t Server::getChannelCount() const
{
	return channels.size();
}

/**
 * Get the hostname for the server.
 */
std::string_view Server::getHostname()
{
	// If we haven't found out the hostname yet, read it from /etc/hostname, or
	// use a default value.
	if (hostname.empty()) {
		hostname.assign("localhost");
		auto openMode = std::ios::in | std::ios::ate;
		std::ifstream file("/etc/hostname", openMode);
		if (file.is_open()) {
			std::string contents(file.tellg(), '\0');
			file.seekg(0);
			if (file.read(contents.data(), contents.size())) {
				if (contents.ends_with('\n'))
					contents.pop_back();
				hostname = contents;
			}
		}
	}
	return hostname;
}
