NAME = RaysThatCast
SOURCES = main.c raycaster.c
OBJECTS = $(SOURCES:.c=.o)
CC = gcc
LIBS = -lSDL2 -lSDL2_gfx -lSDL2_net -lm -ggdb

SNAME = RaysThatCastDS
SSOURCES = server.c
SOBJECTS = $(SSOURCES:.c=.o)
SLIBS = -lSDL2 -lSDL2_net -lm -ggdb

COMPILE_FLAGS = -ggdb -Wall

client: $(OBJECTS)
	$(CC) $^ $(COMPILE_FLAGS) -o $(NAME) $(LIBS)

server: $(SOBJECTS)
	$(CC) $^ $(COMPILE_FLAGS) -o $(SNAME) $(SLIBS)

clean:
	rm -f $(OBJECTS) $(NAME) $(SOBJECTS) $(SNAME)
