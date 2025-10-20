#pragma once

#include <cstdarg>
#include <stdexcept>
#include <string>
#include <string_view>

// ANSI escape codes for nicer terminal output.
#define ANSI_RED	"\x1b[31m"
#define ANSI_GREEN	"\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_RESET	"\x1b[0m"

void safeClose(int& fd);
bool matchIgnoreCase(const char* a, const char* b);
char* nextListItem(char*& list);

// Logging functions.

void log(const std::string_view& string);
inline void log(char chr) { log(std::string_view(&chr, 1)); }
inline void log(char* string) { log(std::string_view(string)); }
inline void log(const char* string) { log(std::string_view(string)); }
inline void log(const std::string& string) { log(std::string_view(string)); }

template <typename Type>
void log(const Type& value)
{
	log(std::to_string(value));
}

template <typename First, typename... Rest>
void log(const First& first, const Rest&... rest)
{
	log(first);
	log(rest...);
}

template <typename... Arguments>
void logInfo(const Arguments&... arguments)
{
	log(ANSI_GREEN "[INFO] " ANSI_RESET, arguments..., "\n");
}

template <typename... Arguments>
void logWarn(const Arguments&... arguments)
{
	log(ANSI_YELLOW "[WARN] " ANSI_RESET, arguments..., "\n");
}

template <typename... Arguments>
void logError(const Arguments&... arguments)
{
	log(ANSI_RED "[ERROR] " ANSI_RESET, arguments..., "\n");
}

/**
 * Helper macro for logging an error message (using logError) and then throwing
 * a runtime_error with information about the function, file and line number
 * where the failure occurred.
 */
#define fail(...) do { \
		logError("Failure in ", __func__, " (", __FILE__, ":", __LINE__, ")"); \
		logError("Reason for failure: ", __VA_ARGS__); \
		throw std::runtime_error("fail() was called"); \
	} while (false)

/**
 * Assert some condition, and throw an exception if it's not true. The error
 * messages includes some information about the condition, the function, file
 * and line number where the assertion failed.
 */
#define assert(condition) do { \
		if (!(condition)) { \
			logError("Assertion failed in ", __func__, " (", __FILE__, ":", __LINE__, ")"); \
			logError("Condition: assert(", #condition, ")"); \
			throw std::runtime_error("assertion failed"); \
		} \
	} while (false)
