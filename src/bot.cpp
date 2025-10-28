#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <fstream>
#include <netdb.h>
#include <sys/epoll.h>

#include "bot.hpp"
#include "irc.hpp"
#include "utility.hpp"

Bot::Bot(const char* name)
	: name(name)
{
}

Bot::~Bot()
{
	safeClose(epoll);
	safeClose(socket);
}

void Bot::run(const char* port, const char* password)
{
	// Install a signal handler for SIGINT, so that the bot can be shut down
	// gracefully with Ctrl + C.
	static volatile sig_atomic_t caughtSignal;
	struct sigaction sa = {};
	sa.sa_handler = [] (int signal) { caughtSignal = signal; };
	sigaction(SIGINT, &sa, nullptr);

	// Open the connection to the server.
	socket = connectToServer(port);

	// Create the epoll instance.
	epoll = epoll_create1(0);
	if (epoll == -1)
		fail("Failed to create epoll instance: ", strerror(errno));

	// Add the socket to the epoll list.
	struct epoll_event event;
	event.events = EPOLLIN | EPOLLOUT | EPOLLET;
	event.data.fd = socket;
	if (epoll_ctl(epoll, EPOLL_CTL_ADD, socket, &event) == -1)
		fail("Failed to add server socket to epoll: ", strerror(errno));

	// Start by sending credentials to the server.
	sendLine("PASS ", password);
	sendLine("NICK ", name);
	sendLine("USER ", name, " 0 * ", name);

	// Run the bot loop.
	log::info("Bot ", name, " running on port ", port);
	while (!disconnected) {

		// Check for available events.
		int eventCount = epoll_wait(epoll, &event, 1, -1);
		if (eventCount == -1) {
			if (errno == EINTR && caughtSignal == SIGINT) {
				std::fprintf(stderr, "\r"); // Just to avoid printing ^C.
				log::info("Interrupted by user");
				break;
			}
			fail("Failed to wait for events: ", strerror(errno));
		}

		// Communicate with the server.
		send();
		receive();
	}
}

int Bot::connectToServer(const char* port)
{
	struct addrinfo *ai = nullptr;

	try {

		// Get address info for the socket.
		struct addrinfo hints = {};
		hints.ai_family   = AF_INET;		// IPv4 only (not IPv6).
		hints.ai_socktype = SOCK_STREAM;	// TCP only (not UDP).
		int status = getaddrinfo(nullptr, port, &hints, &ai);
		if (status != 0)
			fail("getaddrinfo() failed: ", gai_strerror(status));

		// Create the listening socket.
		socket = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (socket == -1)
			fail("socket() failed: ", strerror(errno));

		// Connect to the server.
		if (connect(socket, ai->ai_addr, ai->ai_addrlen) == -1)
			fail("connect failed: ", strerror(errno));
		freeaddrinfo(ai);
	
	// Free resources if any of the preceding syscalls failed.
	} catch (...) {
		freeaddrinfo(ai);
		throw; // Rethrow the same exception.
	}
	return socket;
}

void Bot::send(const std::string_view& string)
{
	output.append(string);
	ssize_t bytes = 1;
	const int sendFlags = MSG_DONTWAIT | MSG_NOSIGNAL;
	while (bytes > 0 && output.find("\r\n") != output.npos) {
		bytes = ::send(socket, output.data(), output.size(), sendFlags);
		if (bytes == -1) {
			if (errno == EAGAIN || errno == ECONNRESET || errno == EPIPE)
				break;
			fail("Failed to send to server: ", strerror(errno));
		}
		output.erase(0, bytes);
	}
}

void Bot::receive()
{
	// Receive data from the server.
	char buffer[512];
	ssize_t bytes = 1;
	while (bytes > 0) {
		bytes = recv(socket, buffer, sizeof(buffer), MSG_DONTWAIT);

		// Handle errors.
		if (bytes == -1) {
			if (errno == EAGAIN || errno == ECONNRESET)
				break;
			fail("Failed to receive from server: ", strerror(errno));

		// Handle client disconnection.
		} else if (bytes == 0) {
			disconnected = true;
			break;

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

void Bot::parseMessage(std::string message)
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

		// Ignore tags and sources (parts starting with an '@' or ':' sign).
		if (message[begin] != '@' && (argc > 0 || message[begin] != ':')) {

			// Issue a warning if there are too many parts.
			if (argc == MAX_MESSAGE_PARTS) {
				log::warn("Message has too many parts: ", message);
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

void Bot::handleMessage(int argc, char** argv)
{
	// A list of dirty words for the bot to react to.
	static const char* const bad_words[] = {
		"shit",
		"piss",
		"fuck",
		"cunt",
		"cocksucker",
		"motherfucker",
		"tits",
		"vscode",
	};

	// Ignore empty commands.
	if (argc == 0)
		return;
	char* command = argv[0];

	// Join any channels the bot is invited to.
	if (argc == 3 && std::strcmp(command, "INVITE") == 0) {
		channels.insert(argv[2]);
		sendLine("JOIN ", argv[2]);
	}

	// Leave any channels the bot is kicked from.
	if (argc == 4 && std::strcmp(command, "KICK") == 0) {
		if (argv[2] == name) {
			channels.erase(argv[1]);
			log::warn("Kicked from ", argv[1]);
		}
	}

	// React to private messages.
	if (argc == 3 && std::strcmp(command, "PRIVMSG") == 0) {
		char* channel = argv[1];
		char* message = argv[2];

		// Check if it's in a channel the bot is joined to.
		if (channels.contains(channel)) {

			// Convert the message to lowercase first.
			for (size_t i = 0; message[i] != '\0'; i++)
				message[i] = std::tolower(message[i]);

			// Look for bad words, and scold the sender if one is found.
			for (const char* word: bad_words) {
				if (std::strstr(message, word)) {
					sendLine("PRIVMSG ", channel, " :NO, BAD WORD!");
					break;
				}
			}
		}
	}
}
