NAME := ircserv
CXXFLAGS := -Wall -Wextra -Werror -std=c++20 -MMD -MP -ggdb

# File names.
SRC := $(wildcard src/**.cpp)      # Source files
OBJ := $(SRC:src/%.cpp=.build/%.o) # Object files
DEP := $(SRC:src/%.cpp=.build/%.d) # Dependency files

# ANSI escape codes
RED    := \x1b[1;31m
GREEN  := \x1b[1;32m
YELLOW := \x1b[1;33m
RESET  := \x1b[0m

all: $(NAME)

$(NAME): $(OBJ)
	@ printf '$(GREEN)Link:\x1b$(RESET) $@\n'
	@ c++ $^ -o $@

.build/%.o: src/%.cpp | .build
	@ printf '$(YELLOW)Compile:$(RESET) $<\n'
	@ c++ -c $< -o $@ $(CXXFLAGS)

.build:
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
	irssi -c localhost -w secret

nc:
	nc -C localhost 6667

.PHONY: all clean fclean re test leaks irssi nc
.SECONDARY: $(OBJ)
-include $(DEP)
