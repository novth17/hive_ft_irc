# Internet Relay Chat - ft_irc

A standards-compliant IRC (Internet Relay Chat) server, implemented in C++.

This project consists of building our own IRC server following the famous 1988-era Internet Relay Chat protocol.
A client implementation is not part of the project — instead, we used real IRC clients (e.g., irssi) to test the server.


## 🎬 Demo

https://github.com/user-attachments/assets/e2a23788-b88c-45b1-a42f-dbeac9704303

---
## 🛠️ Features
The server behaves like a Real™ IRC server, implementing the following core networking and protocol features:

### 🔧 Core Functionality
- Handle multiple clients simultaneously
- Use TCP/IP (IPv4)
- Use non-blocking I/O
- Use a single epoll event loop for all I/O
- Correctly handle partial packets (fragmented messages)
- No forking, no threads
- Does not block, hang, or crash unexpectedly

### 💬 Supported IRC Commands

#### Basic commands
Implemented the essential IRC commands:
```
PASS
NICK
USER
JOIN
PRIVMSG
PING / PONG
Broadcasting messages to all users in a channel
```

#### Channel operator commands
Operators must be able to manage channels using:
```
KICK — remove a user from the channel
INVITE — invite a user to a channel
TOPIC — set or view the channel topic
MODE — adjust channel modes:
    i	  Invite-only channel
    t	  Only operators may change the topic
    k	  Channel key (password)
    o	  Give/take operator privileges
    l	  Set/remove user limit
```

### 🤖 ChatBot
Feature: warning users who use "naughty" words.
Connects to the server as a user, responds to channel invites, replies to specific messages.

----
## 📦 Build & Run

Compile the program with make. Then launch the server with ./ircserv [NETWORK PORT] [PASSWORD] (empty password == no password)
```
make
```

Launch example:
```
./ircserv 6667 secret
```
Then connect using an IRC client. Connect to the server with your IRC client of choice! Our reference client when making the server was irssi.
```
irssi
/connect 127.0.0.1 6667 secret
```

Meaning:
- server: 127.0.0.1
- port: 6667
- password: secret

Optionally, run prudebot with ./ircserv [NETWORK PORT] [PASSWORD] [BOT NICKNAME] at any point after launching the server.

## Credits

- [Eve Keinan](https://github.com/EvAvKein)
- [Axel Boström](https://github.com/datagore)
- [Hien (Lily) Nguyen](https://github.com/novth17)

