NAME := webserv
CPP := c++
SRCDIR := ../../src
INCDIR := ../../src
OBJDIR = ../../bin
TESTDIR := ../

CPPFLAGS = -Wall -Wextra -Werror -std=c++98
DEBUGFLAGS = -g -g -ggdb
SANADDRFLAG = -fsanitize=address
INC_FLAGS := -I ./$(INCDIR)

MAINSOURCE := main.cpp
SOURCE_FILES := \
								Connection.cpp \
								Config.cpp

SOURCES := $(addprefix $(SRCDIR)/,$(SOURCE_FILES))
OBJECTS = $(addprefix $(OBJDIR)/,$(SOURCE_FILES:.cpp=.o))

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(@D)
	$(CPP) $(CPPFLAGS) $(INC_FLAGS) -c $< -o $@

all: $(NAME)

# $(NAME): $(OBJECTS) $(SOURCES) $(MAINSOURCE)
$(NAME):
	rm -f webserv
	c++ main.cpp ../../src/Connection.cpp ../../src/Config.cpp -o webserv
# $(CPP) $(CPPFLAGS) $(INC_FLAGS) $(OBJECTS) $(MAINSOURCE) -o $(NAME) -v

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
