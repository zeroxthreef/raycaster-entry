NAME = RaysThatCast
SOURCES = main.c raycaster.c
OBJECTS = $(SOURCES:.c=.o)
CC = gcc
LIBS = -lSDL2 -lSDL2_gfx -lSDL2_net -lm

SNAME = RaysThatCastDS
SSOURCES = server.c
SOBJECTS = $(SSOURCES:.c=.o)
SLIBS = -lSDL2 -lSDL2_net -lm

COMPILE_FLAGS = -g -Ofast_math -Wall -o

client: $(OBJECTS)
	$(CC) $^ $(COMPILE_FLAGS) $(NAME) $(LIBS)

server: $(SOBJECTS)
	$(CC) $^ $(COMPILE_FLAGS) $(SNAME) $(SLIBS)

clean:
	rm -f $(OBJECTS) $(NAME) $(SOBJECTS) $(SNAME)
