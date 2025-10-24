NAME := ircserv
CXXFLAGS := -Wall -Wextra -Werror -std=c++20 -MMD -MP -ggdb -Iinc

# File names.
SRC := $(wildcard src/*.cpp src/*/*.cpp)	# Source files
OBJ := $(SRC:src/%.cpp=.build/%.o)			# Object files
DEP := $(SRC:src/%.cpp=.build/%.d)			# Dependency files
DIR := $(sort $(dir $(OBJ)))

# ANSI escape codes
RED    := \x1b[1;31m
GREEN  := \x1b[1;32m
YELLOW := \x1b[1;33m
RESET  := \x1b[0m

# Generate a random nick for testing with irssi.
RANDOM_NICK := `shuf /usr/share/dict/words | grep -v "'" | head -1`

all: $(NAME)

$(NAME): $(OBJ) $(DIR)
	@ printf '$(GREEN)Link:\x1b$(RESET) $@\n'
	@ c++ $(OBJ) -o $@

.build/%.o: src/%.cpp
	@ printf '$(YELLOW)Compile:$(RESET) $<\n'
	@ c++ -c $< -o $@ $(CXXFLAGS)

$(DIR):
	@ mkdir -p $@

clean:
	@ printf '$(RED)Remove:$(RESET) .build\n'
	@ $(RM) -r .build

fclean: clean
	@ printf '$(RED)Remove:$(RESET) $(NAME)\n'
	@ $(RM) $(NAME)

re: fclean all

test: all
	./$(NAME) 6667 secret

leaks: all
	valgrind --track-fds=yes ./$(NAME) 6667 secret

irssi:
	irssi -c localhost -w secret -n $(RANDOM_NICK)

nc:
	nc -C localhost 6667

.PHONY: all clean fclean re test leaks irssi nc
.SECONDARY: $(OBJ)
-include $(DEP)
