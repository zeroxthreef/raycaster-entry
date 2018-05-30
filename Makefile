NAME = RaysThatCast
SOURCES = main.c raycaster.c
OBJECTS = $(SOURCES:.c=.o)
CC = gcc
LIBS = -lSDL2 -lSDL2_gfx -lm
COMPILE_FLAGS = -g -Wall -o

all: $(OBJECTS)
	$(CC) $^ $(COMPILE_FLAGS) $(NAME) $(LIBS)

clean:
	rm -f $(OBJECTS) $(NAME)
