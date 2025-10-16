#include <sys/socket.h>
#include <unistd.h>

#include "irc.hpp"

/**
 * Close a file descriptor, but only if it's valid. Then reset it to an invalid
 * file descriptor so that it can't accidentally be closed twice.
 */
void safeClose(int& fd)
{
	if (fd != -1) {
		close(fd);
		fd = -1;
	}
}

/**
 * Write a message to stderr with a message-specific prefix. Used internally by
 * the other logging functions.
 */
void log(const char* prefix, const char* format, va_list& args)
{
	fprintf(stderr, "%s " ANSI_RESET, prefix);
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
}

/**
 * Write a green info message to stderr using printf-style formatting syntax. A
 * line break is added to the end of the message.
 */
void logInfo(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	log(ANSI_GREEN "[INFO]", format, args);
	va_end(args);
}

/**
 * Write a yellow warning message to stderr using printf-style formatting
 * syntax. A line break is added to the end of the message.
 */
void logWarn(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	log(ANSI_YELLOW "[WARN]", format, args);
	va_end(args);
}

/**
 * Write a red error message to stderr using printf-style formatting syntax. A
 * line break is added to the end of the message.
 */
void logError(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	log(ANSI_RED "[ERROR]", format, args);
	va_end(args);
}

/**
 * Send a string to a socket using printf-style formatting syntax. Returns the
 * same value and sets errno the same way as regular send().
 */
int sendf(int socket, const char* format, ...)
{
	// Get the length of the formatted string.
	va_list args;
	va_start(args, format);
	int length = 1 + vsnprintf(nullptr, 0, format, args);
	va_end(args);

	// Format the string into a temporary buffer.
	char buffer[length];
	va_start(args, format);
	vsnprintf(buffer, length, format, args);
	va_end(args);

	// Send the formatted string.
	return send(socket, buffer, length - 1, 0);
}

/**
  * Check if two null-terminated strings match, ignoring lower/upper case.
  */
bool matchIgnoreCase(const char* a, const char* b)
{
	for (; *a || *b; a++, b++)
		if (std::toupper(*a) != std::toupper(*b))
			return false;
	return true;
}
