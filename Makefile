NAME := webserv
CPP := c++
SRCDIR := src
INCDIR := inc
OBJDIR = bin
TESTDIR := test

CPPFLAGS = -Wall -Wextra -Werror -std=c++17 -g -ggdb -fsanitize=undefined # -fsanitize=address
DEBUGFLAGS = -g -g -ggdb
SANADDRFLAG = -fsanitize=address
INC_FLAGS := -I ./$(INCDIR)

MAINSOURCE := $(SRCDIR)/main.cpp
SOURCE_FILES := \
								Config.cpp \
								ConnectionManager.cpp \
								HttpConnection.cpp \
								Logger.cpp \
								Server.cpp \
								Libft.cpp \
								Base.cpp \
								HttpHandle.cpp \
								HttpRequest.cpp \
								DirectoryListing.cpp




SOURCES := $(addprefix $(SRCDIR)/,$(SOURCE_FILES))
OBJECTS = $(addprefix $(OBJDIR)/,$(SOURCE_FILES:.cpp=.o))

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(@D)
	$(CPP) $(CPPFLAGS) $(INC_FLAGS) -c $< -o $@

all: $(NAME)

$(NAME): $(OBJECTS) $(SOURCES) $(MAINSOURCE)
	$(CPP) $(CPPFLAGS) $(INC_FLAGS) $(OBJECTS) $(MAINSOURCE) -o $(NAME)

debug: CPPFLAGS += $(DEBUGFLAGS)
debug: all
sanaddr: CPPFLAGS += $(SANADDRFLAG)
sanaddr: debug all


HEADERS = $(addprefix $(SRCDIR)/,$(SOURCE_FILES:.cpp=.hpp))
format:
	clang-format -Werror -i --style=LLVM $(SOURCES)
	clang-format -Werror -i --style=LLVM $(MAINSOURCE)
	clang-format -Werror -i --style=LLVM $(HEADERS)


clean:
	rm -f $(OBJECTS)

fclean: clean
	rm -f $(NAME)
	rm -rf $(NAME).dSYM
	rm -f test/*/*.out
	rm -rf test/*/*.out.dSYM

re: fclean all

.PHONY: all test clean fclean re
