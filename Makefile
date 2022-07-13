CC	= clang
RM	= rm -f

CFLAGS	= -Wall -Wextra -Werror -ansi \
			-fsanitize=address -fsanitize=undefined \
			-Iinclude \
			$(shell pkg-config --cflags sdl2)
LDFLAGS	=  -lasan -lubsan -lpthread $(shell pkg-config --libs sdl2)

SRCS	= main.c client.c backend/sdl.c
OBJS	= $(addprefix src/, $(SRCS:.c=.o))

.PHONY: all
all: mulderX

mulderX: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	$(RM) $(OBJS)

.PHONY: re
re: clean all
