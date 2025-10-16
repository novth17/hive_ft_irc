#include "irc.hpp"
#include "events.hpp"
#include <signal.h>

void safeClose(int& fd)
{
    if (fd != -1) {
        close(fd);
        fd = -1;
    }
}

Server::Server(const char* port, const char* password)
    : _port(port), _password(password)
{
}

Server::~Server()
{
    printf("Closing connection...\n");
    safeClose(_sockfd);
}

/*
🔹 The flags (level-triggered mode)

EPOLLIN
→ Wake me when this socket is readable.
For the listening socket → new client is ready to be accepted.
For a client socket → client has sent some data.

EPOLLOUT
→ Wake me when this socket is writable.
Used only when we still have pending data in the write buffer.
*/
void Server::handleNewConnection() {
    while (true) {
        sockaddr_in clientAddress;
        socklen_t clientAddressLength = sizeof(clientAddress);

        int clientFd = accept(_serverFd, (struct sockaddr*)&clientAddress, &clientAddressLength);
        if (clientFd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No more pending clients
                break;
            } else {
                std::cerr << "accept() failed: " << strerror(errno) << std::endl;
                break;
            }
        }
        SocketUtils::setNonBlocking(clientFd);

        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = clientFd;

        if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, clientFd, &ev) == -1) {
            std::cerr << "Failed to add client socket to epoll." << std::endl;
            close(clientFd);
            continue;
        }

        std::cout << "Accepted new client, fd=" << clientFd << std::endl;
    }
}

int Server::eventLoop(const char* host, const char* port) {
    printf("Starting with password '%s'\n", _password);

	 // Ignore SIGINT.
    struct sigaction sa = {};
    sa.sa_handler = [] (int) {}; // Do-nothing function.
    sigaction(SIGINT, &sa, nullptr);

	//create epoll
	struct epoll_event epollEvent, events[MAX_EVENTS];

	//create listen socket
    _serverFd = SocketUtils::createListenSocket(host, port, true);
    if (_serverFd == -1) {
        std::cerr << "Failed to create server socket." << std::endl;
        return 1;
    }

	//create epoll instance for serverFD
    _epollFd = epoll_create1(0);
    if (_epollFd == -1) {
        std::cerr << "Failed to create epoll instance." << std::endl;
        close(_serverFd);
        return 1;
    }

    // Add server fd socket to epoll
    epollEvent.events = EPOLLIN;
    epollEvent.data.fd = _serverFd;
    if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, _serverFd, &epollEvent) == -1) {
        std::cerr << "Failed to add server socket to epoll." << std::endl;
        close(_serverFd);
        close(_epollFd);
        return 1;
    }

    std::cout << "Server started. Listening on port " << _port << std::endl;

    while (true) {
        int numberOfReadyEvents = epoll_wait(_epollFd, events, MAX_EVENTS, -1);
        if (numberOfReadyEvents == -1) {
            std::cerr << "Failed to wait for events." << std::endl;
            break;
        }
        for (int i = 0; i < numberOfReadyEvents; ++i) {
            int fd = events[i].data.fd;
            if (fd == _serverFd) {
                // Accept new client
                sockaddr_in clientAddress;
                socklen_t clientAddressLength = sizeof(clientAddress);
                int clientFd = accept(_serverFd, (struct sockaddr*)&clientAddress, &clientAddressLength);
                if (clientFd == -1) {
					throw std::runtime_error("Error: accept()");
                }
                SocketUtils::setNonBlocking(clientFd);

                epollEvent.events = EPOLLIN; // watch for reads
                epollEvent.data.fd = clientFd;
                if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, clientFd, &epollEvent) == -1) {
					close(clientFd);
                    throw std::runtime_error ("Failed to add client socket to epoll.");
                }
                std::cout << "Accepted new client, fd=" << clientFd << std::endl;

            } else {
                // Handle client data
                char buffer[4096];
                int bytesRead = read(fd, buffer, sizeof(buffer) - 1);
                if (bytesRead <= 0) {
                    std::cout << "Client disconnected, fd=" << fd << std::endl;
                    epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, nullptr);
                    close(fd);
                } else {
                    buffer[bytesRead] = '\0';
                    std::cout << "Client(" << fd << ") request:\n" << buffer << std::endl;

                    const char* body = "Hello from epoll!\n";
                    std::string response =
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Length: " + std::to_string(strlen(body)) + "\r\n"
                        "Content-Type: text/plain\r\n"
                        "Connection: close\r\n"
                        "\r\n" +
                        std::string(body);

                    send(fd, response.c_str(), response.size(), 0);

                    // close after response
                    //epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, nullptr);
                   // close(fd);
                }
            }
        }
    }
    close(_serverFd);
    close(_epollFd);
    return 0;
}
