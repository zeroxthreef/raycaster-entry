#ifndef RAYCASTER_H__
#define RAYCASTER_H__
#include <SDL2/SDL.h>
#include <SDL2/SDL2_framerate.h>

#define MAPSCALE 16
#define FOVRENDERDISTANCE 50
#define PI 3.14159265f




typedef struct
{
  unsigned char type;
  unsigned int posx;
  unsigned int posy;
} map_tile_t;

typedef struct
{
  float x;
  float y;
  float velx;
  float vely;
  unsigned char health;
  /* extras */
  unsigned char *name; /* for possible multiplayer */
  unsigned char type;
  SDL_Texture *texture;
} entity_t;

typedef struct
{
  unsigned int width;
  unsigned int height;
  unsigned int win_width;
  unsigned int win_height;
  float cam_angle;
  float cam_velocity;
  float cam_fov;
  float max_distance;
  unsigned char can_debug;
  FPSmanager fpsman;
  entity_t player;
  float camera_height_offset; /* NOTE: can vary up or down. May use with sinewave to simulate walking and dying*/
  float camera_ydiff_max;
  /* NOTE: redundant because it can just scan over the map tiles and search for the type
  unsigned int player_startx;
  unsigned int player_starty;
  */
  map_tile_t *tiles;
  entity_t *entities;
} map_settings_t;

enum
{
  TILE_AIR,
  TILE_PLAYERSTART,
  TILE_STONEWALL,
  TILE_DIRTWALL,
  TILE_WOODWALL,
  TILE_GLASSWALL,
  TILE_DOOR
};


void render(map_settings_t *map);

int raycaster_initbasics(map_settings_t *map);

void raycaster_destroy();

float degrees_to_radians(float deg);

float radians_to_degrees(float rad);

#endif
