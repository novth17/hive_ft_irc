#include <iostream>

#include "log.hpp"

namespace log {

void print(const std::string_view& string)
{
	std::cout << string;
}

void print(char chr)
{
	print(std::string_view(&chr, 1));
}

void print(char* string)
{
	print(std::string_view(string));
}

void print(const char* string)
{
	print(std::string_view(string));
}

void print(const std::string& string)
{
	print(std::string_view(string));
}

} // End of namespace log.
