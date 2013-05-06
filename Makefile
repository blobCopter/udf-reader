CXX      = g++
CXXFLAGS += -W -Wall -Wextra -I.
LDFLAGS  +=

NAME  = udf-reader
SRC   = main.cpp

OBJ   = $(SRC:.cpp=.o)

all : $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
