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

