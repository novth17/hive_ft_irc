#include <climits>
#include <cstring>
#include <iostream>
#include <unistd.h>

#include "utility.hpp"

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
  * Check if two null-terminated strings match, ignoring lower/upper case.
  */
bool matchIgnoreCase(const char* a, const char* b)
{
	for (; *a || *b; a++, b++)
		if (std::toupper(*a) != std::toupper(*b))
			return false;
	return true;
}

/**
 * For a string containing items separated by some delimiter (command by
 * default), null-terminate the first item in the list and return it. Also move
 * the list forward to the next item in the list.
 */
char* nextListItem(char*& list, const char* delimiter)
{
	char* firstItem = list;
	list += strcspn(list, delimiter);
	if (*list == ',')
		*list++ = '\0';
	return firstItem;
}

/**
 * Convert a string to an integer. Return true if the conversion was successful,
 * or false if the value is not a valid int.
 */
bool parseInt(const char* input, int& output)
{
	assert(input != nullptr);

	// Convert the value to a long.
	char* end = nullptr;
	long value = std::strtol(input, &end, 10);

	// Check that it's in the valid range of port numbers.
	if (errno == ERANGE || value < INT_MIN || value > INT_MAX)
		return false;

	// Check that the whole string was used (no extra non-digits).
	if (*input == '\0' || *end != '\0')
		return false;
	output = static_cast<int>(value);
	return true;
}
