
#pragma once

#include "irc.hpp"
#include "SocketUtils.hpp"
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <iostream>

#include <string.h>
#include <sys/epoll.h>
#include "Connection.hpp"

constexpr int MAX_CLIENTS = 10;
constexpr int MAX_EVENTS = 10;
