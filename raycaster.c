#include "raycaster.h"

#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

SDL_Window *mainwin = NULL;
SDL_Window *debugwin = NULL;
SDL_Renderer *mainrenderer = NULL;
SDL_Renderer *debugrenderer = NULL;
unsigned char can_debug = 1;
SDL_Rect temprect; /* general purpose rect */


static short internal_asprintf(char **string, const char *fmt, ...) /* took this from my DisC library. Too lazy to make it again */
{
  va_list list;
  char *tempString = NULL;
  char *oldstring = NULL;
  int size = 0;

  if(*string != NULL)
  {
    oldstring = *string;
  }

  va_start(list, fmt);
  size = vsnprintf(tempString, 0, fmt, list);
  va_end(list);
  va_start(list, fmt);

  if((tempString = malloc(size + 1)) != NULL)
  {
    if(vsnprintf(tempString, size + 1, fmt, list) != -1)
    {
      *string = tempString;
      if(oldstring != NULL)
      {
        free(oldstring);
      }
      return size;
    }
    else
    {
      *string = NULL;
      if(oldstring != NULL)
      {
        free(oldstring);
      }
      return -1;
    }
  }
  va_end(list);
}

static void internal_drawDebug(map_settings_t *map)
{
  SDL_version version;
  char *fps_string = NULL;
  char *playerx_string = NULL;
  char *playery_string = NULL;
  char *playerangle_string = NULL;
  char *compile_string = NULL;
  char *link_string = NULL;
  stringRGBA(mainrenderer, 0, 0, "FPS: TODO", 255, 255, 255, 255); /* TODO do it right */
  internal_asprintf(&playerx_string, "player x: %f", map->player.x);
  stringRGBA(mainrenderer, 0, 8, playerx_string, 255, 255, 255, 255);
  internal_asprintf(&playery_string, "player y: %f", map->player.y);
  stringRGBA(mainrenderer, 0, 16, playery_string, 255, 255, 255, 255);
  internal_asprintf(&playerangle_string, "player angle: %f", map->cam_angle);
  stringRGBA(mainrenderer, 0, 24, playerangle_string, 255, 255, 255, 255);
  SDL_VERSION(&version);
  internal_asprintf(&compile_string, "compiled %d.%d.%d", version.major, version.minor, version.patch);
  stringRGBA(mainrenderer, 0, 32, compile_string, 255, 255, 255, 255);
  SDL_GetVersion(&version);
  internal_asprintf(&link_string, "compiled %d.%d.%d", version.major, version.minor, version.patch);
  stringRGBA(mainrenderer, 0, 40, link_string, 255, 255, 255, 255);

  /* free everything */
  free(fps_string);
  free(playerx_string);
  free(playery_string);
  free(playerangle_string);
  free(compile_string);
  free(link_string);
}

/* global functions */


void render(map_settings_t *map)
{
  /* for the main window */
  /* ================================== */
  /* draw the ceiling and the floor. This is very wip. Need to set ceiling and floor color from map file */
  size_t i, ii;
  float angle;
  float distance;
  float fov0;
  float fov1;

  temprect.x = 0;
  temprect.y = 0;
  temprect.w = map->win_width;
  temprect.h = map->win_height/2;
  SDL_SetRenderDrawColor(mainrenderer, 160, 10, 3, 255);
  SDL_RenderFillRect(mainrenderer, &temprect);
  temprect.x = 0;
  temprect.y = map->win_height/2;
  temprect.w = map->win_width;
  temprect.h = map->win_height/2;
  SDL_SetRenderDrawColor(mainrenderer, 6, 100, 210, 255);
  SDL_RenderFillRect(mainrenderer, &temprect);

  for(i = 0; i < map->width * map->height; i++)
  {
    if(map->tiles[i].type != TILE_AIR && map->tiles[i].type != TILE_PLAYERSTART) /* filter out things that shouldnt interferre with the raycast */
    {
      angle = atan2f(map->tiles[i].posy - map->player.y, map->tiles[i].posx - map->player.x);
      distance = sqrt((map->tiles[i].posx - map->player.x) * (map->tiles[i].posx - map->player.x) + (map->tiles[i].posy - map->player.y) * (map->tiles[i].posy - map->player.y)) * MAPSCALE;
      fov0 = map->cam_angle - map->cam_fov/2;
      fov1 = map->cam_angle + map->cam_fov/2;

      if(angle > degrees_to_radians(fov0) && angle < degrees_to_radians(fov1))
      {
        /* do the real raycasting tests. Send out a ray within the fov at the width number of pixels */

        /* temporary line and distance persp test */
        float linex = (((radians_to_degrees(angle) - map->cam_angle) * map->win_width) / map->cam_fov) + map->win_width/2;
        float liney0 = map->win_height/2 - (map->win_height * 2 / distance); /* top so it goes negative */
        float liney1 = map->win_height/2 + (map->win_height * 2 / distance); /* bottom so it goes positive */
        thickLineRGBA(mainrenderer, linex, liney0, linex, liney1, 4, 160, 200, 210, 255);








      }

    }
  }
  /*
  Sint16 xp[] = {20, 60, 30, 50};
  Sint16 yp[] = {100, 200, 200, 100};

  filledPolygonRGBA(mainrenderer, xp, yp, 4, 255, 0, 0, 255);
  */

  //printf("framerate %d\n", SDL_getFramecount(&map->fpsman));


  if(can_debug)
    internal_drawDebug(map);
  /* ================================== */
  SDL_SetRenderDrawColor(mainrenderer, 0, 0, 0, 255);
  SDL_RenderPresent(mainrenderer);
  SDL_RenderClear(mainrenderer);
  /* ================================== */

  /* for the debug window */
  /* ================================== */
  if(can_debug)
  {
    size_t i;
    for(i = 0; i < map->width * map->height; i++)
    {
      SDL_SetRenderDrawColor(debugrenderer, map->tiles[i].type * 120 + 20, map->tiles[i].type * 200 + 50, map->tiles[i].type * 400 + 213, 200);
      temprect.x = ((i % map->width) * MAPSCALE);
      temprect.y = ((i / map->height) * MAPSCALE);
      temprect.w = MAPSCALE;
      temprect.h = MAPSCALE;

      SDL_RenderFillRect(debugrenderer, &temprect);
      SDL_SetRenderDrawColor(debugrenderer, map->tiles[i].type * 120 + 10, map->tiles[i].type * 200 + 25, map->tiles[i].type * 400 + 120, 200);
      SDL_RenderDrawRect(debugrenderer, &temprect);
    }

    /* draw the player, fov, and direction */
    SDL_SetRenderDrawColor(debugrenderer, 220, 255, 160, 255);
    temprect.x = map->player.x  * MAPSCALE - 3;
    temprect.y = map->player.y * MAPSCALE - 3;
    temprect.w = 7;
    temprect.h = 7;
    SDL_RenderFillRect(debugrenderer, &temprect);

    SDL_SetRenderDrawColor(debugrenderer, 255, 70, 10, 255);
    SDL_RenderDrawLine(debugrenderer, map->player.x * MAPSCALE , map->player.y * MAPSCALE , (cos(degrees_to_radians(map->cam_angle + 0.0f + map->cam_fov/2)) * FOVRENDERDISTANCE) + map->player.x * MAPSCALE, (sin(degrees_to_radians(map->cam_angle + 0.0f + map->cam_fov/2)) * FOVRENDERDISTANCE) + map->player.y * MAPSCALE);
    SDL_RenderDrawLine(debugrenderer, map->player.x * MAPSCALE , map->player.y * MAPSCALE , (cos(degrees_to_radians(map->cam_angle + 0.0f - map->cam_fov/2)) * FOVRENDERDISTANCE) + map->player.x * MAPSCALE, (sin(degrees_to_radians(map->cam_angle + 0.0f - map->cam_fov/2)) * FOVRENDERDISTANCE) + map->player.y * MAPSCALE);

    SDL_SetRenderDrawColor(debugrenderer, 255, 7, 1, 255);
    SDL_RenderDrawLine(debugrenderer, map->player.x * MAPSCALE , map->player.y * MAPSCALE , (cos(degrees_to_radians(map->cam_angle)) * FOVRENDERDISTANCE * 1.3) + map->player.x * MAPSCALE, (sin(degrees_to_radians(map->cam_angle)) * FOVRENDERDISTANCE * 1.3) + map->player.y * MAPSCALE);

    /* draw lines to everything in the fov */

    for(i = 0; i < map->width * map->height; i++)
    {
      if(map->tiles[i].type != TILE_AIR && map->tiles[i].type != TILE_PLAYERSTART)
      {
        float angle = atan2f(map->tiles[i].posy - map->player.y, map->tiles[i].posx - map->player.x);
        float distance = sqrt((map->tiles[i].posx - map->player.x) * (map->tiles[i].posx - map->player.x) + (map->tiles[i].posy - map->player.y) * (map->tiles[i].posy - map->player.y)) * MAPSCALE;
        float fov0 = map->cam_angle - map->cam_fov/2;
        float fov1 = map->cam_angle + map->cam_fov/2;

        //printf("fov1 %f fov2 %f", fov0, fov1);
        if(angle > degrees_to_radians(fov0) && angle < degrees_to_radians(fov1))
        {
          SDL_RenderDrawLine(debugrenderer, map->player.x * MAPSCALE , map->player.y * MAPSCALE , (cos(angle) * distance) + map->player.x * MAPSCALE, (sin(angle) * distance) + map->player.y * MAPSCALE);
          //printf("angle %f fov1 %f fov 2 %f facing", radians_to_degrees(angle), fov0, fov1);
        }
        //printf("\n");
      }



    }


    /* ================================== */
    SDL_SetRenderDrawColor(debugrenderer, 0, 0, 0, 0);
    SDL_RenderPresent(debugrenderer);
    SDL_RenderClear(debugrenderer);
  }
  /* ================================== */
}

int raycaster_initbasics(map_settings_t *map)
{
  map->camera_height_offset = 0;
  mainwin = SDL_CreateWindow("hey, whats up rey castor - entry for the gaia raycaster challenge 2018 - 0x3F", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, map->win_width, map->win_height, SDL_WINDOW_SHOWN);
  debugwin = SDL_CreateWindow("map debug window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 550, 550, SDL_WINDOW_SHOWN);

  mainrenderer = SDL_CreateRenderer(mainwin, -1, SDL_RENDERER_ACCELERATED);
  debugrenderer = SDL_CreateRenderer(debugwin, -1, SDL_RENDERER_ACCELERATED);

  return 0;
}

void render_draw4poly(int corner0x, int corner0y, int corner1x, int corner1y, int corner2x, int corner2y, int corner3x, int corner3y)
{
  /* terrible fill system but whatever */
  size_t i;

}

void raycaster_destroy()
{
  SDL_DestroyRenderer(mainrenderer);
  SDL_DestroyRenderer(debugrenderer);
  SDL_DestroyWindow(mainwin);
  SDL_DestroyWindow(debugwin);
  /* possibly clean up any other textures */
}

float degrees_to_radians(float deg)
{
  return deg * PI / 180.0f;
}

float radians_to_degrees(float rad)
{
return rad * 180.0f / PI;
}
