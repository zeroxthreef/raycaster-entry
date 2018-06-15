NAME = RaysThatCast
#NAME = RaysThatCast.exe
#I had to include the sdl_gfx lib seperately because it wont cross compile under mingw on linux as its own library
SOURCES = main.c raycaster.c lib/SDL2_gfxPrimitives.c lib/SDL2_framerate.c lib/SDL2_imageFilter.c lib/SDL2_rotozoom.c
OBJECTS = $(SOURCES:.c=.o)
CC = gcc
#CC = i686-w64-mingw32-gcc
LIBS = -lSDL2 -lSDL2_net -lm

SNAME = RaysThatCastDS
SSOURCES = server.c
SOBJECTS = $(SSOURCES:.c=.o)
SLIBS = -lSDL2 -lSDL2_net -lm -ggdb

COMPILE_FLAGS = -ffast-math -O3 -Wall

client: $(OBJECTS)
	$(CC) $(COMPILE_FLAGS) $^ -o $(NAME) $(LIBS)

server: $(SOBJECTS)
	$(CC) $(COMPILE_FLAGS) $^ -o $(SNAME) $(SLIBS)

clean:
	rm -f $(OBJECTS) $(NAME) $(SOBJECTS) $(SNAME)
