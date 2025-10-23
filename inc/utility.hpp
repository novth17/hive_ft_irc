#pragma once

#include <stdexcept>

// ANSI escape codes for nicer terminal output.
#define ANSI_RED	"\x1b[31m"
#define ANSI_GREEN	"\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_RESET	"\x1b[0m"

/**
 * Helper macro for logging an error message (using logError) and then throwing
 * a runtime_error with information about the function, file and line number
 * where the failure occurred.
 */
#define fail(...) do { \
		log::error("Failure in ", __func__, " (", __FILE__, ":", __LINE__, ")"); \
		log::error("Reason for failure: ", __VA_ARGS__); \
		throw std::runtime_error("fail() was called"); \
	} while (false)

/**
 * Assert some condition, and throw an exception if it's not true. The error
 * messages includes some information about the condition, the function, file
 * and line number where the assertion failed.
 */
#define assert(condition) do { \
		if (!(condition)) { \
			log::error("Assertion failed in ", __func__, " (", __FILE__, ":", __LINE__, ")"); \
			log::error("Condition: assert(", #condition, ")"); \
			throw std::runtime_error("assertion failed"); \
		} \
	} while (false)

void safeClose(int& fd);
bool matchIgnoreCase(const char* a, const char* b);
char* nextListItem(char*& list, const char* delimiter = ",");
