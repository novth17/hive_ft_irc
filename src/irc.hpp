#pragma once

#include <map>
#include <stdexcept>

#define PORT_MAX 65535 // The maximum allowed port number.
#define MAX_BACKLOG 20 // Maximum length of the pending connection queue.
#define MAX_MESSAGE_PARTS 15 // Maximum number of parts/params in one message.

// ANSI escape codes for nicer terminal output.
#define ANSI_RED	"\x1b[31m"
#define ANSI_GREEN	"\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_RESET	"\x1b[0m"

// This attribute makes clang check the format string of a printf-like function,
// and issue a warning if it's invalid. The argument should be the 1-based index
// of the format string parameter.
#define CHECK_FORMAT(x) __attribute__((format(printf, x, x + 1)))

struct Client
{
	int socket = -1;			// The socket used for the client's connection.
	std::string nick;			// The client's nickname.
	std::string user;			// The client's user name.
	bool isOperator = false;	// True if the client is an operator.
	std::string input;			// Buffered data from recv().
	std::string output;			// Buffered data for send().
};

struct Channel
{
	std::string name;	// The name of the channel.
	std::string topic;	// The current topic for the channel.
};

class Server
{
public:
	Server(const char* port, const char* password);
	~Server();

	void eventLoop(const char* host, const char* port);

private:
	void parseMessage(std::string message);
	void handleMessage(char** params, int paramCount);

	// Handlers for specific messages.
	void handleUser(char** params, int paramCount);
	void handleNick(char** params, int paramCount);
	void handlePass(char** params, int paramCount);
	void handleCap(char** params, int paramCount);
	void handleJoin(char** params, int paramCount);

	const char* _port = nullptr;
	const char* _password = nullptr;
	int _serverFd = -1;
	int _epollFd = -1;
	std::map<int, Client> _clients;
	std::map<std::string, Channel> _channels;
};

// utility.cpp
void safeClose(int& fd);
void logInfo(const char* format, ...) CHECK_FORMAT(1);
void logWarn(const char* format, ...) CHECK_FORMAT(1);
void logError(const char* format, ...) CHECK_FORMAT(1);
int sendf(int socket, const char* format, ...) CHECK_FORMAT(2);
bool matchIgnoreCase(const char* a, const char* b);

/**
 * Throw an exception with an error message given using printf-style formatting
 * syntax. Throws std::runtime_error by default; other exception types can be
 * used, for example `throwf<std::logic_error>("message")`.
 */
template <typename ErrorType = std::runtime_error>
CHECK_FORMAT(1)
void throwf(const char* format, ...)
{
	// Get the length of the formatted string.
	va_list args;
	va_start(args, format);
	int length = 1 + vsnprintf(nullptr, 0, format, args);
	va_end(args);

	// Format the string into a temporary buffer.
	std::string message(length, '\0');
	va_start(args, format);
	vsnprintf(message.data(), length, format, args);
	va_end(args);

	// Throw an exception with the formatted error message.
	throw ErrorType(message);
}
