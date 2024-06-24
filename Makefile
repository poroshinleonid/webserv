NAME := webserv
CPP := c++
SRCDIR := src
INCDIR := inc
OBJDIR = bin
TESTDIR := test

CPPFLAGS = -Wall -Wextra -Werror -std=c++98
DEBUGFLAGS = -g -g -ggdb
SANADDRFLAG = -fsanitize=address
INC_FLAGS := -I ./$(INCDIR)

MAINSOURCE := $(SRCDIR)/main.cpp
SOURCE_FILES := \
								ServerCore.cpp \
								Config.cpp

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


clean:
	rm -f $(OBJECTS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all test clean fclean re
