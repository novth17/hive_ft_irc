#pragma once

#include <string>
#include <string_view>

// ANSI escape codes for nicer terminal output.
#define ANSI_RED	"\x1b[31m"
#define ANSI_GREEN	"\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_RESET	"\x1b[0m"

namespace log {

void print(const std::string_view& string);
void print(char chr);
void print(char* str);
void print(const char* str);
void print(const std::string& str);

template <typename Type>
void print(const Type& value)
{
	print(std::to_string(value));
}

template <typename First, typename... Rest>
void print(const First& first, const Rest&... rest)
{
	print(first);
	print(rest...);
}

template <typename... Arguments>
void info(const Arguments&... arguments)
{
	print(ANSI_GREEN "[INFO] " ANSI_RESET, arguments..., "\n");
}

template <typename... Arguments>
void warn(const Arguments&... arguments)
{
	print(ANSI_YELLOW "[WARN] " ANSI_RESET, arguments..., "\n");
}

template <typename... Arguments>
void error(const Arguments&... arguments)
{
	print(ANSI_RED "[ERROR] " ANSI_RESET, arguments..., "\n");
}

} // End of namespace log.
