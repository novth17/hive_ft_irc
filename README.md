# FT_IRC

My 12th project at 42 Network's Hive Helsinki 🐝

A standards-compliant IRC (Internet Relay Chat) server, implemented in C++

> [!TIP]
> If you're at a 42 school and doing this project: It's genuinely so much better to ask fellow students instead of reading online solutions ✨

---

## Description

An IRC (Internet Relay Chat) server implemented in C++20.
Compliant with [RFC 1459](https://datatracker.ietf.org/doc/html/rfc1459) and [Horse Docs](https://modern.ircdocs.horse/) standards.
Made for the reference IRC client [Irssi](https://irssi.org/) (with additional testing in [Netcat](https://en.wikipedia.org/wiki/Netcat)).

Core functionality:
- A single event loop, with non-blocking IO through event polling - no threads or subprocesses.
- Buffers incoming requests, handling fragmented input.
- Compatible with IPv4.
- Supports file transfers via the DCC (Direct Client-to-Client) protocol.
- Secondary bot program, warning users who use naughty words: Connects to the server as a user, responds to channel invites, replies to specific messages.

The server supports the following IRC commands:
- Basic commands:
  - Timeout checks:
    - `PING`
  - Registration:
    - `PASS`
    - `USER`
    - `NICK` (including nickname changing after registration)
  - Server & channel access:
    - `JOIN`
    - `PART`
    - `QUIT`
  - Messaging:
    - `PRIVMSG` (including messages to channels)
    - `NOTICE`
  - Info:
    - `TOPIC`
    - `MOTD`
    - `LIST`
    - `NAMES`
    - `WHO`
    - `LUSERS`
- Channel operators commands:
  - `KICK`
  - `INVITE`
  - `MODE` with the following channel settings:
    - `i` - toggle invite-only channel
    - `t` - toggle topic-editing limited to operators
    - `k` - set/remove channel key (password)
    - `i` - give/take operator privileges
    - `l` - set/remove user limit

## Usage

> [!NOTE]
> Code was written and tested for Linux (using Hive's Ubuntu iMacs)

1. Compile the program with `make`
2. Launch the server with `./ircserv [NETWORK PORT] [PASSWORD]` (empty password == no password)
3. Connect to the server with your IRC client of choice! Our reference client when making the server was `irssi`.

Optionally, run prudebot with `./ircserv [NETWORK PORT] [PASSWORD] [BOT NICKNAME]` at any point after launching the server.


## Credits

- [Eve Keinan](https://github.com/EvAvKein)
- [Axel Boström](https://github.com/datagore)
- [Hien (Lily) Nguyen](https://github.com/novth17)

