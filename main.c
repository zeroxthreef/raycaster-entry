/*
this project is by 0x3F
TODO multiple levels

*/

#include "raycaster.h"

#include <SDL2/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


int init();
void logic();
void destroy();
void loop();
int parse_map(char *location, map_settings_t *map);
int clean_map(map_settings_t *map);

SDL_Event event;
map_settings_t map;
unsigned char quit = 0;


int main(int argc, char **argv)
{
  if(init())
    destroy();

  loop();

  destroy();

  return 0;
}

int init()
{
  printf("initializing\n");
  SDL_Init(SDL_INIT_EVERYTHING);

  SDL_initFramerate(&map.fpsman);
  SDL_setFramerate(&map.fpsman, 60);



  map.cam_angle = 0; /* need to set these inside the .map map files */
  map.cam_fov = 60;
  map.win_width = 800;
  map.win_height = 600;

  raycaster_initbasics(&map);


  /* temporary map init TODO, make an array of level names and order*/

  parse_map("level.map", &map);

  return 0;
}

void logic()
{
  /* map.player.x += map.player.velx / 12; */
  /* map.player.y += map.player.vely / 12; */
  map.player.x -= (map.player.vely / 12) * cos(degrees_to_radians(map.cam_angle));
  map.player.y -= (map.player.vely / 12) * sin(degrees_to_radians(map.cam_angle));

  map.player.x += (map.player.velx / 12) * cos(degrees_to_radians(map.cam_angle + 90));
  map.player.y += (map.player.velx / 12) * sin(degrees_to_radians(map.cam_angle + 90));

  map.cam_angle += map.cam_velocity * 2; /* This is actually a terrible "fix" */
  if(map.cam_angle < -180.0f)
    map.cam_angle = 180;
  if(map.cam_angle > 180.0f)
    map.cam_angle = -180;
}

void destroy()
{
  printf("closing\n");
  raycaster_destroy();
  SDL_Quit();
}

void loop()
{
  printf("running\n");
  while(!quit)
  {
    /* events */
    while(SDL_PollEvent(&event))
    {
      if(event.type == SDL_QUIT)
        quit = (unsigned char)1;
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
          quit = (unsigned char)1;
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
      }
    }
    /* logic */
    logic();
    /* rendering */
    render(&map);
    /* delaying */
    SDL_framerateDelay(&map.fpsman); /* decided to use whats already here. I already know how to make a fixed timestep gameloop */
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
