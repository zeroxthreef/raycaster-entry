#ifndef RAYCASTER_H__
#define RAYCASTER_H__
#include <SDL2/SDL.h>
#include <SDL2/SDL2_framerate.h>

#define MAPSCALE 16
#define FOVRENDERDISTANCE 50
#define PI 3.14159265f
#define MAXSERVERPACKET 2048




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
  float angle;
  unsigned char health;
  /* extras */
  Uint64 id;
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
  unsigned char thread_count; /* set to 1 if only single threading */
  unsigned char *threadnum_done; /* have each thread access its index to signal the screen to render */
  unsigned char quit;
  entity_t player;
  unsigned char display_connectbox;
  char *ip_str;
  unsigned char text_entry_mode;
  float camera_height_offset; /* NOTE: can vary up or down. May use with sinewave to simulate walking and dying*/
  float camera_ydiff_max;
  /* NOTE: redundant because it can just scan over the map tiles and search for the type
  unsigned int player_startx;
  unsigned int player_starty;
  */
  map_tile_t *tiles;
  unsigned long entitity_count;
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

enum
{
  ENTITY_PLAYER_0, /* stick figure */
  ENTITY_PLAYER_1, /* some kind of cat?? */
  ENTITY_PLAYER_2, /* a dragon thing */
  ENTITY_MONSTER0
};

enum /* networking flags */
{
  FLAG_SERVER_RETURN_ID, /* client gets their ID */
  FLAG_GET_EVENTCOUNT, /* NOTE: crucial to implement right because the client needs to know how many events to check for */
  FLAG_GET_ID_DETAILS, /* client gets entity at id#'s details like name, type, original position, angle, and health NOTE: it is sent out every time anything in the entity id updates */
  FLAG_RETURN_DETAILS, /* client gives it's details every ime it changes anything */
  FLAG_REQUEST_RESPAWN, /* when the client dies and wants to respawn */
  FLAG_REQUEST_PROJECTILE, /* will return an id with the name "projectile" and damage the health of everything that it touches and lasts a set amount of time */
  FLAG_GET_ID_DELETE, /* when a player leaves or a projectile is freed */
  FLAG_REQUEST_MAPCHANGE, /* a client requests a mapfile that may or may not be in the servers dir by name*/
  FLAG_UPLOAD_MAP, /* a client can upload maps of a certain size limit to the server under a name */
  FLAG_RECIEVE_MESSAGE /* the client recieves a message from the server like the MOTD or when clients disconnect */
};

short internal_asprintf(char **string, const char *fmt, ...); /* took this from my DisC library. Too lazy to make it again */

int init_threads(map_settings_t *map);

void render(map_settings_t *map, unsigned char threadnum);

void changeDebugwinState(unsigned char st);

int raycaster_initbasics(map_settings_t *map);

void raycaster_destroy();

float degrees_to_radians(float deg);

float radians_to_degrees(float rad);

#endif
