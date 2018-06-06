/*
this project is by 0x3F
TODO multiple levels

*/

#include "raycaster.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

#define MAXENTITIES 100


int init();
void logic();
void destroy();
void loop();
int parse_map(char *location, map_settings_t *map);
int clean_map(map_settings_t *map);

SDL_Event event;
map_settings_t map;

short connected = 0;



int main(int argc, char **argv)
{
  if(init())
    destroy();

  loop();

  destroy();

  return 0;
}

Uint8 *internal_dataSerialize(Uint64 **len)
{
  /* only actually do something if any data has changed or its the first time */
  static char *oldx = NULL, *oldy = NULL, *oldang = NULL;
  char *px_str = NULL, *py_str = NULL, *pa_str = NULL;
  static unsigned char first = 0, oldtype = 0;
  static Uint8 *data = NULL;

  internal_asprintf(&px_str, "%f", map.player.x);
  internal_asprintf(&py_str, "%f", map.player.y);
  internal_asprintf(&pa_str, "%f", map.cam_angle);

  Uint64 serialized_data = sizeof(Uint8) + 10 + sizeof(Uint8) + (sizeof(Uint16) + strlen(px_str) + 1) + (sizeof(Uint16) + strlen(py_str) + 1) + (sizeof(Uint16) + strlen(pa_str) + 1) + sizeof(Uint8); /* messageheader name, type, position(x,y), angle, and health NOTE yeah, I know its not network byte order */


  if(!first)
  {
    data = malloc(serialized_data);
    if(data == NULL)
    {
      printf("mem alloc error\n");
      return NULL;
    }
    internal_asprintf(&oldx, "a");
    internal_asprintf(&oldy, "a");
    internal_asprintf(&oldang, "a");
    if(oldx == NULL || oldy == NULL || oldang == NULL)
    {
      printf("mem alloc error\n");
      return NULL;
    }
    first = 1;
  }


  data[0] = FLAG_RETURN_DETAILS; /* copy header */
  memcpy(&data[sizeof(Uint8)], map.player.name, 10); /* copy name */
  memcpy(&data[sizeof(Uint8) + 10], &map.player.type, sizeof(Uint8)); /* copy type */

          data[sizeof(Uint8) + 10 + sizeof(Uint8)] = strlen(px_str) + 1; /* copy position x length*/
  memcpy(&data[sizeof(Uint8) + 10 + sizeof(Uint8) + sizeof(Uint16)], px_str, strlen(px_str) + 1); /* copy position x*/

          data[sizeof(Uint8) + 10 + sizeof(Uint8) + (sizeof(Uint16) + strlen(px_str) + 1)] = strlen(py_str) + 1; /* copy position y length*/
  memcpy(&data[sizeof(Uint8) + 10 + sizeof(Uint8) + (sizeof(Uint16) + strlen(px_str) + 1) + sizeof(Uint16)], py_str, strlen(py_str) + 1); /* copy position y*/

          data[sizeof(Uint8) + 10 + sizeof(Uint8) + (sizeof(Uint16) + strlen(px_str) + 1) + (sizeof(Uint16) + strlen(py_str) + 1)] = strlen(pa_str) + 1; /* copy angle length*/
  memcpy(&data[sizeof(Uint8) + 10 + sizeof(Uint8) + (sizeof(Uint16) + strlen(px_str) + 1) + (sizeof(Uint16) + strlen(py_str) + 1) + sizeof(Uint16)], pa_str, strlen(pa_str) + 1); /* copy angle*/

  memcpy(&data[sizeof(Uint8) + 10 + sizeof(Uint8) + (sizeof(Uint16) + strlen(px_str) + 1) + (sizeof(Uint16) + strlen(py_str) + 1) + (sizeof(Uint16) + strlen(pa_str) + 1)], &map.player.health, sizeof(Uint8)); /* copy health */


  //printf("px: %s py: %s a: %s\n", &data[sizeof(Uint8) + 10 + sizeof(Uint8) + sizeof(Uint16)], &data[sizeof(Uint8) + 10 + sizeof(Uint8) + (sizeof(Uint16) + strlen(px_str) + 1) + sizeof(Uint16)], &data[sizeof(Uint8) + 10 + sizeof(Uint8) + (sizeof(Uint16) + strlen(px_str) + 1) + (sizeof(Uint16) + strlen(py_str) + 1) + sizeof(Uint16)]);
  /* set both and send if its changed only */

  if(strcmp(px_str, oldx) != 0 || strcmp(py_str, oldy) != 0 || strcmp(pa_str, oldang) != 0 || map.player.type != oldtype) /* type, posx, posy, angle */
  {
    *len = serialized_data;
    /* yep, I gave up on comparing floats... I'd had enough of it never working */
    internal_asprintf(&oldx, "%f", map.player.x);
    internal_asprintf(&oldy, "%f", map.player.y);
    internal_asprintf(&oldang, "%f", map.cam_angle);


    oldtype = map.player.type;
    return data;
  }
  return NULL;
}

int network(void *data)
{
  IPaddress ip;
  TCPsocket sock;
  Uint8 *cdata = NULL;
  Uint64 dlen = 0;
  Uint64 events, i = 0;

  size_t tlen;
  Uint8 tdata[MAXSERVERPACKET];

  while(!map.quit)
  {
    if(connected == -1) /* connect to a server now */
    {

      printf("connecting to server %s\n", map.ip_str);

      if(SDLNet_ResolveHost(&ip, map.ip_str, 19191) == -1)
      {
        printf("host resolve error\n");
        SDL_Delay(600);
        continue;
      }

      if((sock = SDLNet_TCP_Open(&ip)) == NULL)
      {
        printf("connection error\n");
        SDL_Delay(600);
        continue;
      }

      printf("connected\n");
      /* do setup */
      tlen = SDLNet_TCP_Recv(sock, tdata, MAXSERVERPACKET);
      if(tlen == 0)
      {
        printf("connection error\n");
      }
      if(tdata[0] == FLAG_SERVER_RETURN_ID)
      {
        printf("server responded correctly and player entity id is %lu\n", (Uint64)tdata[1]);
        map.player.id = (Uint64)tdata[1];
        map.display_connectbox = 0;
        connected = 1;

      }

    }
    else if(connected == 1)
    {
      /* do the things to send data out */
      if((cdata = internal_dataSerialize(&dlen)) != NULL)
      {
        printf("updating server with client data\n");
        if(SDLNet_TCP_Send(sock, cdata, dlen) < dlen)
        {
          printf("data error. Client disconencted\n");
          connected = 0;
        }
      }

      /* listen to what the server has to say */

      /* check how many events there are to parse */
      tlen = SDLNet_TCP_Recv(sock, tdata, MAXSERVERPACKET);
      if(tlen == 0)
      {
        printf("connection error\n");
        connected = 0;
      }
      events = (Uint64)tdata[1];
      printf("events: %lu\n", (Uint64)tdata[1]);
      if(tdata[0] == FLAG_GET_EVENTCOUNT)
      {
        i = 0;
        while(i < events)
        {
          tlen = SDLNet_TCP_Recv(sock, tdata, MAXSERVERPACKET);
          if(tlen == 0)
          {
            printf("connection error\n");
            connected = 0;
          }

          /* TODO: make the server send all of the entities it has in the first round after a player connects */
          switch((Uint8)tdata[0])
          {
            case FLAG_GET_ID_DETAILS:

            if(map.entities[tdata[tlen - sizeof(Uint64)]].name != NULL)
              free(map.entities[tdata[tlen - sizeof(Uint64)]].name);

            map.entities[tdata[tlen - sizeof(Uint64)]].name = SDL_strdup(&cdata[sizeof(Uint8)]);
            map.entities[tdata[tlen - sizeof(Uint64)]].type = cdata[sizeof(Uint8) + 10];
            /* TODO make sure the end of each string is null terminated */
            map.entities[tdata[tlen - sizeof(Uint64)]].x = atof(&cdata[sizeof(Uint8) + 10 + sizeof(Uint8) + sizeof(Uint16)]);
            map.entities[tdata[tlen - sizeof(Uint64)]].y = atof(&cdata[sizeof(Uint8) + 10 + sizeof(Uint8) + sizeof(Uint16) + ( (Uint16)cdata[sizeof(Uint8) + 10 + sizeof(Uint8)] ) + sizeof(Uint16)]);
            map.entities[tdata[tlen - sizeof(Uint64)]].angle = atof(&cdata[sizeof(Uint8) + 10 + sizeof(Uint8) + sizeof(Uint16) + ( (Uint16)cdata[sizeof(Uint8) + 10 + sizeof(Uint8)] ) + sizeof(Uint16) + ((Uint16)cdata[sizeof(Uint8) + 10 + sizeof(Uint8) + sizeof(Uint16) + ( (Uint16)cdata[sizeof(Uint8) + 10 + sizeof(Uint8)] )]) + sizeof(Uint16)]);
            map.entities[tdata[tlen - sizeof(Uint64)]].health = cdata[dlen - sizeof(Uint8)];

            break;
            case FLAG_GET_ID_DELETE:

            break;
          }

        }
      }
      else
        printf("bad flag order\n");
    }
    SDL_Delay(25);
  }
  return 0;
}

int init()
{
  printf("initializing\n");
  SDL_Init(SDL_INIT_EVERYTHING);


  map.quit = 0;
  map.cam_angle = 0; /* need to set these inside the .map map files */
  map.cam_fov = 60;
  map.win_width = 800;
  map.win_height = 600;
  map.max_distance = 100.0f;
  map.can_debug = 0;
  map.camera_height_offset = 0.0f;
  map.camera_ydiff_max = 4.0f;
  map.thread_count = 1;
  /*map.thread_count = SDL_GetCPUCount(); NOTE: didnt finish this on time. Probably could get it to work if I really wanted to */
  map.threadnum_done = calloc(map.thread_count, sizeof(unsigned char));
  map.player.name = calloc(10, sizeof(char));
  map.ip_str = calloc(17, sizeof(char));
  map.player.health = 100;
  map.player.type = 0;
  map.display_connectbox = 0;
  map.text_entry_mode = 0;
  map.entities = calloc(MAXENTITIES, sizeof(entity_t));

  raycaster_initbasics(&map);

  /* temporary map init TODO, make an array of level names and order*/

  parse_map("level.map", &map);

  if(map.thread_count > 1)
    init_threads(&map);

  if(SDL_CreateThread(network, "network thread", NULL) == NULL)
    printf("couldn't start network thread\n");

  return 0;
}

void logic()
{
  static float i = 0.0f;
  if(map.player.health != 0)
  {
    /* map.player.x += map.player.velx / 12; */
    /* map.player.y += map.player.vely / 12; */
    map.player.x -= (map.player.vely / 12) * cos(degrees_to_radians(map.cam_angle));
    map.player.y -= (map.player.vely / 12) * sin(degrees_to_radians(map.cam_angle));

    map.player.x += (map.player.velx / 12) * cos(degrees_to_radians(map.cam_angle + 90));
    map.player.y += (map.player.velx / 12) * sin(degrees_to_radians(map.cam_angle + 90));

    map.cam_angle += map.cam_velocity * 2; /* This is actually a terrible "fix" */
  }
  if(map.can_debug)/* snapping makes things jumpy */
  {
    if(map.cam_angle < -180.0f)
      map.cam_angle = 180;
    if(map.cam_angle > 180.0f)
      map.cam_angle = -180;
  }

  if(map.player.velx != 0 || map.player.vely != 0) /* camera bob */
  {
    map.camera_height_offset = sin(i);
    i+= 0.3;
  }
  if(!map.text_entry_mode && map.display_connectbox == 3)
  {
    connected = -1;
  }
  else if(!map.text_entry_mode && map.display_connectbox > 0)
    map.display_connectbox = 0;


}

void destroy()
{
  printf("closing\n");
  raycaster_destroy();
  SDLNet_Quit();
  SDL_Quit();
}

void loop()
{
	int lastTime = SDL_GetTicks(), skipTime = 0, currentTime = 0, lastframeLag = 0, frameskipNum = 0;
  printf("running\n");

  while(!map.quit)
  {
    static unsigned char pressed = 0;
    /* events */
    while(SDL_PollEvent(&event))
    {
      if(event.type == SDL_QUIT)
        map.quit = (unsigned char)1;

      if(!map.text_entry_mode)
      {
        if(event.type == SDL_KEYDOWN)
        {
          if(event.key.keysym.sym == SDLK_w)
            map.player.vely = -1;
          if(event.key.keysym.sym == SDLK_a)
            map.player.velx = -1;
          if(event.key.keysym.sym == SDLK_s)
            map.player.vely = 1;
          if(event.key.keysym.sym == SDLK_d)
            map.player.velx = 1;
          if(event.key.keysym.sym == SDLK_LEFT)
            map.cam_velocity = -1;
          if(event.key.keysym.sym == SDLK_RIGHT)
            map.cam_velocity = 1;
          if(event.key.keysym.sym == SDLK_ESCAPE)
            map.quit = (unsigned char)1;
          /* general purpose keys */
          if(event.key.keysym.sym == SDLK_BACKSLASH)
          {
            map.player.type += 1;
            map.player.type %= 3;
          }
          if(event.key.keysym.sym == SDLK_BACKQUOTE)
          {
            if(!pressed)
            {
              map.can_debug += 1;
              map.can_debug %= 2;
              printf("debug toggled %d\n", map.can_debug);
              changeDebugwinState(map.can_debug);
              pressed = 1;
            }
          }
          if(event.key.keysym.sym == SDLK_F1)
          {
            map.text_entry_mode = 1;
            map.display_connectbox = 1;
          }
        }
        if(event.type == SDL_KEYUP)
        {
          if(event.key.keysym.sym == SDLK_w)
            map.player.vely = 0;
          if(event.key.keysym.sym == SDLK_a)
            map.player.velx = 0;
          if(event.key.keysym.sym == SDLK_s)
            map.player.vely = 0;
          if(event.key.keysym.sym == SDLK_d)
            map.player.velx = 0;
          if(event.key.keysym.sym == SDLK_LEFT)
            map.cam_velocity = 0;
          if(event.key.keysym.sym == SDLK_RIGHT)
            map.cam_velocity = 0;
          if(event.key.keysym.sym == SDLK_BACKQUOTE)
          {
            pressed = 0;
          }
        }
      }
      else
      {
        SDL_StartTextInput();
        static short pressed = 0;
        if(event.type == SDL_KEYDOWN)
        {
          if(event.key.keysym.sym == SDLK_ESCAPE)
            map.text_entry_mode = 0;
          if(map.display_connectbox == 1)
          {
            if(event.key.keysym.sym == SDLK_BACKSPACE)
              if(strlen(map.ip_str) != 0)
                map.ip_str[strlen(map.ip_str) - 1] = 0x00;
            if(event.key.keysym.sym == SDLK_RETURN)
            if(!pressed)
            {
              map.display_connectbox++;
              pressed++;
            }
          }
          if(map.display_connectbox == 2)
          {
            if(event.key.keysym.sym == SDLK_BACKSPACE)
              if(strlen(map.player.name) != 0)
                map.player.name[strlen(map.player.name) - 1] = 0x00;
            if(event.key.keysym.sym == SDLK_RETURN)
            {
              if(!pressed)
              {
                map.display_connectbox++; /* set this one more so the logic knows to connect */
                map.text_entry_mode = 0;
                SDL_StopTextInput();
                pressed++;
              }
            }
          }

        }
        else if(event.type == SDL_KEYUP)
        {
          if(map.display_connectbox == 1)
          {
            if(event.key.keysym.sym == SDLK_RETURN)
            if(pressed)
            {
              pressed = 0;
            }
          }
          if(map.display_connectbox == 2)
          {
            if(event.key.keysym.sym == SDLK_RETURN)
            {
              if(pressed)
              {
                pressed = 0;
              }
            }
          }

        }
        else if(event.type = SDL_TEXTINPUT)
        {
          if(map.display_connectbox == 1)
          {
            if(strlen(map.ip_str) < 17)
              strncat(map.ip_str, event.text.text, 1);
          }
          if(map.display_connectbox == 2)
          {
            if(strlen(map.player.name) < 10)
              strncat(map.player.name, event.text.text, 1);
          }
        }
      }
    }
    /* logic */
    logic();

    /* rendering */
    if(!lastframeLag)
    {
      if(map.thread_count == 1)
        render(&map, 0);
      else
      {
        /* tell all threads that its ok to render */
      }
    }
    else
    {
      lastframeLag = 0;
    }
    lastTime += 1000/60;
		currentTime = SDL_GetTicks();

		skipTime = lastTime - currentTime;

    /* delaying */
    if(skipTime >= 0)
    {
			SDL_Delay(skipTime);
		}
    else
    {
			printf("running %dms behind on rendering\n", abs(skipTime));
      if(frameskipNum < 200) /* only 200 max skipped frames per round */
      {
        lastframeLag = 1;
        frameskipNum++;
      }
      else
      {
        frameskipNum = 0;
      }

		}
  }
}

int parse_map(char *location, map_settings_t *map)
{
  unsigned char offset;
  unsigned char *mapdata = NULL;
  unsigned char *mapdata_semicolon_tokenized = NULL;
  unsigned char *map_config = NULL;
  unsigned char *map_tiles = NULL;
  unsigned char *nextptr;
  unsigned char *mapdata_tiles_tokenized = NULL;
  size_t mapdata_size = 0;
  size_t tile_increment = 0;


  SDL_RWops *mapfile = SDL_RWFromFile(location, "r");

  if(mapfile == NULL)
    return 1;

  mapdata_size = (size_t)mapfile->size(mapfile);


  printf("found %s at %lu bytes, parsing\n", location, mapdata_size);

  if((mapdata = calloc(mapdata_size + 1, sizeof(unsigned char))) == NULL)
  {
    SDL_RWclose(mapfile);
    return 1;
  }

  SDL_RWread(mapfile, mapdata, mapdata_size, 1);

  /*printf("map: %s\n", mapdata);*/

  if(strncmp(mapdata, "map", 3) != 0) /* verify that it is a map */
  {
    printf("invalid map\n");
    SDL_RWclose(mapfile);
    return 1;
  }
  else
    printf("valid map\n");

  mapdata_semicolon_tokenized = strtok(mapdata, ";");

  while(mapdata_semicolon_tokenized != NULL)
  {
    /* check for keywords */
    if(strncmp(mapdata_semicolon_tokenized, "\n", 1) == 0)
      offset = 1;
    else
      offset = 0;
    if(strncmp(mapdata_semicolon_tokenized + offset, "map", 3) == 0)
      map_config = mapdata_semicolon_tokenized + offset;
    else if(strncmp(mapdata_semicolon_tokenized + offset, "BEGIN_MAPDATA", strlen("BEGIN_MAPDATA")) == 0)
      map_tiles = mapdata_semicolon_tokenized + offset;


    mapdata_semicolon_tokenized = strtok(NULL, ";");
  }

  /* printf("map config: %s map tiles: %s\n", map_config, map_tiles); */

  map->width = strtol(map_config + 4, &nextptr, 10);
  map->height = strtol(nextptr + 1, NULL, 10);


  if((map->tiles = malloc(((map->width * map->height) + (map->width * map->height) /2) * sizeof(map->tiles))) == NULL) /* did something stupid, but whatever works and doesnt print stacktrace */
  {
    printf("malloc error\n");
    SDL_RWclose(mapfile);
    return 1;
  }

  mapdata_tiles_tokenized = strtok(map_tiles, ",");

  while(mapdata_tiles_tokenized != NULL)
  {
    if(strncmp(mapdata_tiles_tokenized, "\n", 1) == 0)
      offset = 1;
    else
      offset = 0;
    if(strncmp(mapdata_tiles_tokenized + offset, "BEGIN_MAPDATA", strlen("BEGIN_MAPDATA")) != 0 && strncmp(mapdata_tiles_tokenized + offset, "END_MAPDATA", strlen("END_MAPDATA")) != 0)
    {
      /* actually parse map tiles */
      map->tiles[tile_increment].posx = tile_increment % map->width;
      map->tiles[tile_increment].posy = tile_increment / map->height;
      map->tiles[tile_increment].type = (unsigned char)strtol(mapdata_tiles_tokenized + offset, NULL, 10);
      if(map->tiles[tile_increment].type == TILE_PLAYERSTART)
      {
        map->player.x = (float)map->tiles[tile_increment].posx + 0.5;
        map->player.y = (float)map->tiles[tile_increment].posy + 0.5;
        printf("%f %f\n", map->player.x, map->player.y);
      }

      /*printf("%lu aa:%s, %d --- %d %d %d\n", tile_increment, mapdata_tiles_tokenized + offset, (map->width * map->height), map->tiles[tile_increment].posx, map->tiles[tile_increment].posy, map->tiles[tile_increment].type); */
      tile_increment++;
    }

    mapdata_tiles_tokenized = strtok(NULL, ",");
  }


  printf("parsed map data --- width: %u height: %u\n", map->width, map->height);

  free(mapdata);
  SDL_RWclose(mapfile);
  return 0;
}

int clean_map(map_settings_t *map)
{

  return 0;
}
