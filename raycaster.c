#include "raycaster.h"

/* the images */
#include "hud0.h"
#include "hud1.h"
#include "hud2.h"
#include "died.h"
#include "space.h"

#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

SDL_Window *mainwin = NULL;
SDL_Window *debugwin = NULL;
SDL_Renderer *mainrenderer = NULL;
SDL_Renderer *debugrenderer = NULL;
SDL_Rect temprect; /* general purpose rect */
SDL_Texture *background = NULL;

typedef struct
{
  unsigned char id;
  map_settings_t *map;
} thread_data_t;


short internal_asprintf(char **string, const char *fmt, ...) /* took this from my DisC library. Too lazy to make it again */
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

  if((tempString = calloc(1, size + 1)) != NULL)
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

static void internal_colorCalc(map_settings_t *map, float distance, unsigned char face, unsigned char type, unsigned char isplayer, unsigned char *r, unsigned char *g, unsigned char *b)
{
  float ambient_light = 0.5;
  float north_south_percent = -0.1;
  float east_west_percent = 0.7;

  float pre_distance = map->max_distance - distance;


  /* configure all types and their colors here. This gives appearance */
  if(!isplayer)
  {
    switch(type)
    {
      case TILE_STONEWALL:
        *r = 64;
        *g = 64;
        *b = 64;
      break;
      case TILE_DIRTWALL:
        *r = 153;
        *g = 77;
        *b = 0;
      break;
      case TILE_WOODWALL:
        *r = 255;
        *g = 140;
        *b = 26;
      break;
      case TILE_GLASSWALL: /* was going to add transparency, but realized I'd have to do a second test for rays and I dont want to spend any more time on this really */
        *r = 102;
        *g = 255;
        *b = 179;
      break;
      case TILE_DOOR:
        *r = 153;
        *g = 102;
        *b = 0;
      break;
    }
  }
  else
  {
    switch(type)
    {
      case ENTITY_PLAYER_0:
        if(face == 0)
        {
          *r = 120;
          *g = 120;
          *b = 120;
        }
        else
        {
          *r = 64;
          *g = 64;
          *b = 64;
        }
      break;
      case ENTITY_PLAYER_1:
        if(face == 0)
        {
          *r = 255;
          *g = 120;
          *b = 150;
        }
        else
        {
          *r = 255;
          *g = 165;
          *b = 0;
        }
      break;
      case ENTITY_PLAYER_2:
        if(face == 0)
        {
          *r = 255;
          *g = 150;
          *b = 140;
        }
        else
        {
          *r = 68;
          *g = 200;
          *b = 68;
        }
      break;
      case ENTITY_MONSTER0:
        if(face == 0)
        {
          *r = 255;
          *g = 100;
          *b = 100;
        }
        else
        {
          *r = 250;
          *g = 12;
          *b = 20;
        }
      break;
    }
  }

  /* settings for lighting. Should probably make this dynamic but whatever. Dont really need day night cycles in a silly test */
  if(!isplayer)
  {
    switch(face)
    {
      case 2: /* north */
        if(north_south_percent > 0.0f)
        {
          *r *= (unsigned int)fabs(north_south_percent) + ambient_light;
          *g *= (unsigned int)fabs(north_south_percent) + ambient_light;
          *b *= (unsigned int)fabs(north_south_percent) + ambient_light;
        }
      break;
      case 0: /* south */
        if(north_south_percent < 0.0f)
        {
          *r *= (unsigned int)fabs(north_south_percent) + ambient_light;
          *g *= (unsigned int)fabs(north_south_percent) + ambient_light;
          *b *= (unsigned int)fabs(north_south_percent) + ambient_light;
        }
      break;
      case 1: /* east */
        if(east_west_percent > 0.0f)
        {
          *r *= (unsigned int)fabs(east_west_percent) + ambient_light;
          *g *= (unsigned int)fabs(east_west_percent) + ambient_light;
          *b *= (unsigned int)fabs(east_west_percent) + ambient_light;
        }
      break;
      case 3: /* west */
        if(east_west_percent < 0.0f)
        {
          *r *= (unsigned int)fabs(east_west_percent) + ambient_light;
          *g *= (unsigned int)fabs(east_west_percent) + ambient_light;
          *b *= (unsigned int)fabs(east_west_percent) + ambient_light;
        }
      break;
    }
  }
  else
  {
    switch(face)
    {
      case 2: /* north */
        if(north_south_percent > 0.0f)
        {
          *r *= (unsigned int)fabs(north_south_percent) + ambient_light;
          *g *= (unsigned int)fabs(north_south_percent) + ambient_light;
          *b *= (unsigned int)fabs(north_south_percent) + ambient_light;
        }
      break;
      case 0: /* south */
        if(north_south_percent < 0.0f)
        {
          *r *= (unsigned int)fabs(north_south_percent) + ambient_light;
          *g *= (unsigned int)fabs(north_south_percent) + ambient_light;
          *b *= (unsigned int)fabs(north_south_percent) + ambient_light;
        }
      break;
      case 1: /* east */
        if(east_west_percent > 0.0f)
        {
          *r *= (unsigned int)fabs(east_west_percent) + ambient_light;
          *g *= (unsigned int)fabs(east_west_percent) + ambient_light;
          *b *= (unsigned int)fabs(east_west_percent) + ambient_light;
        }
      break;
      case 3: /* west */
        if(east_west_percent < 0.0f)
        {
          *r *= (unsigned int)fabs(east_west_percent) + ambient_light;
          *g *= (unsigned int)fabs(east_west_percent) + ambient_light;
          *b *= (unsigned int)fabs(east_west_percent) + ambient_light;
        }
      break;
    }
  }

  /* do distance falloff... Maybe logarithmic? No, thats was a stupid waste of time */

  pre_distance /= map->max_distance;


  *r *= pre_distance;
  *g *= pre_distance;
  *b *= pre_distance;

}

static int internal_testRayIntersection(float *intersectx, float *intersecty, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
  /* code taken http://paulbourke.net/geometry/pointlineplane/javascript.txt ... Was desperate. Writing with 30 minutes left :(*/

  if ((x1 == x2 && y1 == y2) || (x3 == x4 && y3 == y4))
  {
		return 0;
	}

  float denominator = ((y4 - y3) * (x2 - x1) - (x4 - x3) * (y2 - y1));


  if (abs(denominator) == 0)
  {
		return 0;
	}

  float ua = ((x4 - x3) * (y1 - y3) - (y4 - y3) * (x1 - x3)) / denominator;
	float ub = ((x2 - x1) * (y1 - y3) - (y2 - y1) * (x1 - x3)) / denominator;

  if (ua < 0 || ua > 1 || ub < 0 || ub > 1)
  {
		return 0;
	}

  *intersectx = x1 + ua * (x2 - x1);
	*intersecty = y1 + ua * (y2 - y1);

  return 1;
}

static void internal_drawDebug(map_settings_t *map, unsigned char when)
{
  SDL_version version;
  static unsigned int start = 0;
  char *fps_string = NULL;
  char *playerx_string = NULL;
  char *playery_string = NULL;
  char *playerangle_string = NULL;
  char *compile_string = NULL;
  char *link_string = NULL;

  internal_asprintf(&fps_string, "fps: %d", 1000/(SDL_GetTicks() - start));
  stringRGBA(mainrenderer, 0, 0, fps_string, 255, 255, 255, 255); /* TODO do it right */
  if(when)
  {
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
  }


  /*diff = SDL_GetTicks() - start; */

  /* free everything */
  free(fps_string);
  if(when)
  {
    free(playerx_string);
    free(playery_string);
    free(playerangle_string);
    free(compile_string);
    free(link_string);
  }


  start = SDL_GetTicks();
}

static int internal_threadDoWork(void *data)
{
  /* do work until done and wait for frame to finish then go again */
  thread_data_t *tdata = (thread_data_t *)data;

  /* do the render loop in here */
  while(!tdata->map->quit)
  {
    render(tdata->map, tdata->id);
    printf("rendering\n");
    tdata->map->threadnum_done[tdata->id - 1] = 1;
  }

  printf("exitting %d\n", tdata->id);

  free(tdata);
  return 0;
}

static int internal_createTexHeader(SDL_Renderer *renderer, SDL_Texture **tex, const unsigned char *dataPtr, unsigned int len){
  SDL_Surface *loadSurface = NULL;
  int pass = 1;
  loadSurface = SDL_LoadBMP_RW(SDL_RWFromMem(dataPtr, len), 0);
  if(loadSurface == NULL)
  {
    pass--;
  }
  SDL_SetColorKey(loadSurface, SDL_TRUE, SDL_MapRGB(loadSurface->format, 255, 0, 128));

  *tex = SDL_CreateTextureFromSurface(renderer, loadSurface);
  SDL_SetTextureBlendMode(*tex, SDL_BLENDMODE_BLEND);
  SDL_FreeSurface(loadSurface);
  return pass;
}

static void internal_drawGUI(map_settings_t *map)
{
  /* draw skin */
  static unsigned char lastSkin = -1; /* wrap around */
  static SDL_Texture *tex = NULL, *death = NULL;

  if(death == NULL)
    internal_createTexHeader(mainrenderer, &death, died_bmp, died_bmp_len);

  if(lastSkin != map->player.type)
  {
    if(tex != NULL)
      SDL_DestroyTexture(tex);

    switch(map->player.type)
    {
      case ENTITY_PLAYER_0:
        internal_createTexHeader(mainrenderer, &tex, hud0_bmp, hud0_bmp_len);
      break;
      case ENTITY_PLAYER_1:
        internal_createTexHeader(mainrenderer, &tex, hud1_bmp, hud1_bmp_len);
      break;
      case ENTITY_PLAYER_2:
        internal_createTexHeader(mainrenderer, &tex, hud2_bmp, hud2_bmp_len);
      break;
    }
    lastSkin = map->player.type;
  }

  SDL_RenderCopy(mainrenderer, tex, NULL, NULL);

  /* draw text */
  stringRGBA(mainrenderer, map->win_width - 490, 0, "Player type (press \"\\\" to cycle player type): ", 255, 255, 255, 255);
  switch(map->player.type)
  {
    case ENTITY_PLAYER_0:
      stringRGBA(mainrenderer, map->win_width - 120, 0, "a stick figure", 255, 255, 255, 255);
    break;

    case ENTITY_PLAYER_1:
      stringRGBA(mainrenderer, map->win_width - 120, 0, "a cat", 255, 255, 255, 255);
    break;

    case ENTITY_PLAYER_2:
      stringRGBA(mainrenderer, map->win_width - 120, 0, "a dragon", 255, 255, 255, 255);
    break;
  }

  stringRGBA(mainrenderer, map->win_width - 490, 10, "Press F1 to connect to a server", 255, 255, 255, 255);

  if(map->player.health == 0)
    SDL_RenderCopy(mainrenderer, death, NULL, NULL);

}

void internal_RenderConnectbox(map_settings_t *map)
{
  char *temp = NULL;

  temprect.x = map->win_width/2 - 250 + 10;
  temprect.y = map->win_height/2 - 40 + 10;
  temprect.w = 500;
  temprect.h = 80;

  SDL_SetRenderDrawColor(mainrenderer, 68, 68, 68, 100);
  SDL_RenderFillRect(mainrenderer, &temprect);

  temprect.x = map->win_width/2 - 250;
  temprect.y = map->win_height/2 - 40;
  temprect.w = 500;
  temprect.h = 80;

  SDL_SetRenderDrawColor(mainrenderer, 108, 108, 108, 100);
  SDL_RenderFillRect(mainrenderer, &temprect);

  if(map->display_connectbox == 1)
  {
    internal_asprintf(&temp, "Server IP(enter IPv4 only. No domain names): %s", map->ip_str);
    stringRGBA(mainrenderer, map->win_width/2 - 240, map->win_height / 2, temp, 255, 255, 255, 255);
  }
  else if(map->display_connectbox == 2)
  {
    internal_asprintf(&temp, "Nickname (10 chars max): %s", map->player.name);
    stringRGBA(mainrenderer, map->win_width/2 - 240, map->win_height / 2, temp, 255, 255, 255, 255);
  }
  else
  {
    free(temp);
    stringRGBA(mainrenderer, map->win_width/2 - 240, map->win_height / 2, "Connecting...", 255, 255, 255, 255);
  }
}

/* global functions */

int init_threads(map_settings_t *map)
{
  unsigned char i, returnval = 0;
  thread_data_t *tdata;
  SDL_Thread *temp = NULL;
  printf("creating render %d threads\n", map->thread_count);

  for(i = 0; i < map->thread_count; i++)
  {
    tdata = calloc(1, sizeof(thread_data_t));/* remember to free this from within the thread */
    tdata->id = i + 1;
    tdata->map = map;

    temp = SDL_CreateThread(internal_threadDoWork, "renderthread", tdata);
    if(temp == NULL)
    {
      returnval = 1;
      printf("couldnt create renderthread %d\n", i);
    }
  }

  return returnval;
}


void render(map_settings_t *map, unsigned char threadnum) /* not the actual thread function. Just call this from threads */
{
  /* for the main window */
  /* ================================== */
  /* draw the ceiling and the floor. This is very wip. Need to set ceiling and floor color from map file */
  size_t i, ii, j, k;
  float angle;
  float distance;
  float fov0;
  float fov1;
  float pointx[4];
  float pointy[4];
  float closestx, closesty, closestdistance;
  float lastx, lasty, firstx, firsty;
  float interx, intery;
  unsigned char face; /*1 of 4 */
  unsigned char last_type, isplayer;
  unsigned char r, g, b;
  unsigned char canrender;

  /* TODO: only render the lines of the screen for the current rays. Remember to predict the placement */
  /* render old sky
  temprect.x = 0;
  temprect.y = 0;
  temprect.w = map->win_width;
  temprect.h = map->win_height/2;
  SDL_SetRenderDrawColor(mainrenderer, 51, 153, 255, 255);
  SDL_RenderFillRect(mainrenderer, &temprect);
  temprect.x = 0;
  temprect.y = map->win_height/2;
  temprect.w = map->win_width;
  temprect.h = map->win_height/2;
  SDL_SetRenderDrawColor(mainrenderer, 0, 102, 0, 255);
  SDL_RenderFillRect(mainrenderer, &temprect);
  */
  for(i = 0; i < 3; i++)
  {
    if(!i)
    temprect.x = -((int)map->cam_angle*(int)map->win_width/(int)360) % 800 - 800;
    else if(i == 1)
    temprect.x = -((int)map->cam_angle*(int)map->win_width/(int)360) % 800;
    else if(i == 2)
    temprect.x = -((int)map->cam_angle*(int)map->win_width/(int)360) % 800 + 800;

    temprect.y = 0;
    temprect.w = 800;
    temprect.h = 600;

    SDL_RenderCopy(mainrenderer, background, NULL, &temprect);
  }
  /*
  int gap = abs(100 - map->max_distance) * map->win_height/1000;
  if(gap < 0)
   gap = 0;
  else if(gap > 255)
    gap = 255;

  for(i = 0; i < map->win_height; i++)
  {

    if(i <= map->win_height/2 - gap)
    {
      SDL_SetRenderDrawColor(mainrenderer, -(i * 255)/map->win_height/2- gap, -(i * 255)/map->win_height/2- gap, -(i * 255)/map->win_height/2- gap, 255);
      SDL_RenderDrawLine(mainrenderer, 0, i, map->win_width, i);
    }
    else if(i >= map->win_height/2 + gap)
    {
      SDL_SetRenderDrawColor(mainrenderer, (i * 255)/map->win_height/2- gap, (i * 255)/map->win_height/2- gap, (i * 255)/map->win_height/2- gap, 255);
      SDL_RenderDrawLine(mainrenderer, 0, i, map->win_width, i);
    }

  }
  */
  for(i = 0; i < map->win_height/2; i++)
  {
    SDL_SetRenderDrawColor(mainrenderer, (i * 255)/map->win_height/2, (i * 255)/map->win_height/2, (i * 255)/map->win_height/2, 255);
    SDL_RenderDrawLine(mainrenderer, 0, i + map->win_width / 2 - 90, map->win_width, i + map->win_width / 2 - 90);

  }


  for(j = 0; j < map->win_width; j++)
  {
    if(j % map->thread_count == threadnum && map->thread_count > 1)
    {
      canrender = 1;
    }
    else if(map->thread_count == 1)
      canrender = 1;
    else
    {
      canrender = 0;
    }

    if(canrender)
    {
      closestdistance = map->max_distance;

      for(i = 0; i < map->width * map->height; i++)
      {
        /*
        angle = atan2f(map->tiles[i].posy - map->player.y, map->tiles[i].posx - map->player.x) + degrees_to_radians(float deg);
        fov0 = map->cam_angle - map->cam_fov/2 + map->cam_fov;
        fov1 = map->cam_angle + map->cam_fov/2 + map->cam_fov;
        printf("f %f %f %f\n", degrees_to_radians(fov0), angle, degrees_to_radians(fov1));
        if(degrees_to_radians(fov0) <= angle && degrees_to_radians(fov1) >= angle)
        */
        if(map->tiles[i].type != TILE_AIR && map->tiles[i].type != TILE_PLAYERSTART) /* filter out things that shouldnt interferre with the raycast */
        {
          pointx[0] = map->tiles[i].posx;/* top left */
          pointy[0] = map->tiles[i].posy;
          pointx[1] = map->tiles[i].posx + 1.0f;/* top right */
          pointy[1] = map->tiles[i].posy;
          pointx[2] = map->tiles[i].posx + 1.0f;/* bottom right */
          pointy[2] = map->tiles[i].posy + 1.0f;
          pointx[3] = map->tiles[i].posx;/* bottom left */
          pointy[3] = map->tiles[i].posy + 1.0f;

          for(ii = 0; ii < 4; ii++)/* generate the vertecies and faces */
          {
            angle = (map->cam_angle - map->cam_fov/2) + ((map->cam_fov * j) / map->win_width);
            distance = sqrt((pointx[ii] - map->player.x) * (pointx[ii] - map->player.x) + (pointy[ii] - map->player.y) * (pointy[ii] - map->player.y));
            fov0 = map->cam_angle - map->cam_fov/2;
            fov1 = map->cam_angle + map->cam_fov/2;


            if(ii != 0) /* skip over 0 because I use last and current NOTE: remember to set a lowest distance variable and only change it if the distance gets closer. might wanna set the vertex too*/
            {
              if(internal_testRayIntersection(&interx, &intery, lastx, lasty, pointx[ii], pointy[ii], map->player.x, map->player.y, map->max_distance * cos(degrees_to_radians(angle)),  map->max_distance * sin(degrees_to_radians(angle))))
              {
                distance = sqrt((interx - map->player.x) * (interx - map->player.x) + (intery - map->player.y) * (intery - map->player.y));
                if(distance < closestdistance)
                {
                  closestdistance = distance;
                  closestx = interx;
                  closesty = intery;
                  face = (unsigned char)ii;
                  last_type = map->tiles[i].type;
                  isplayer = 0;

                }
              }


              lastx = pointx[ii];
              lasty = pointy[ii];
            }
            else
            {
              lastx = pointx[0];
              lasty = pointy[0];
              firstx = lastx;
              firsty = lasty;

            if(internal_testRayIntersection(&interx, &intery, lastx, lasty, pointx[3], pointy[3], map->player.x, map->player.y, map->max_distance * cos(degrees_to_radians(angle)),  map->max_distance * sin(degrees_to_radians(angle))))
              {
                distance = sqrt((interx - map->player.x) * (interx - map->player.x) + (intery - map->player.y) * (intery - map->player.y));
                if(distance < closestdistance)
                {
                  closestdistance = distance;
                  closestx = interx;
                  closesty = intery;
                  face = (unsigned char)ii;
                  last_type = map->tiles[i].type;
                  isplayer = 0;
                }
              }
            }
          }
        }



      }
      /* now test for entities */
      for(k = 0; k < MAXENTITIES; k++)
      {
        if(map->entities[k].enabled)
        {
          pointx[0] = map->entities[k].x + (ENTITYWIDTH * cos(degrees_to_radians(map->entities[k].angle + 45)));/* top left */
          pointy[0] = map->entities[k].y + (ENTITYWIDTH * sin(degrees_to_radians(map->entities[k].angle + 45)));
          pointx[1] = map->entities[k].x + (ENTITYWIDTH * cos(degrees_to_radians(map->entities[k].angle + 45 + 90)));/* top right */
          pointy[1] = map->entities[k].y + (ENTITYWIDTH * sin(degrees_to_radians(map->entities[k].angle + 45 + 90)));
          pointx[2] = map->entities[k].x + (ENTITYWIDTH * cos(degrees_to_radians(map->entities[k].angle + 45 + 180)));/* bottom right */
          pointy[2] = map->entities[k].y + (ENTITYWIDTH * sin(degrees_to_radians(map->entities[k].angle + 45 + 180)));
          pointx[3] = map->entities[k].x + (ENTITYWIDTH * cos(degrees_to_radians(map->entities[k].angle + 45 + 270)));/* bottom left */
          pointy[3] = map->entities[k].y + (ENTITYWIDTH * sin(degrees_to_radians(map->entities[k].angle + 45 + 270)));

          for(ii = 0; ii < 4; ii++)/* generate the vertecies and faces */
          {
            angle = (map->cam_angle - map->cam_fov/2) + ((map->cam_fov * j) / map->win_width);
            distance = sqrt((pointx[ii] - map->player.x) * (pointx[ii] - map->player.x) + (pointy[ii] - map->player.y) * (pointy[ii] - map->player.y));
            fov0 = map->cam_angle - map->cam_fov/2;
            fov1 = map->cam_angle + map->cam_fov/2;


            if(ii != 0) /* skip over 0 because I use last and current NOTE: remember to set a lowest distance variable and only change it if the distance gets closer. might wanna set the vertex too*/
            {
              if(internal_testRayIntersection(&interx, &intery, lastx, lasty, pointx[ii], pointy[ii], map->player.x, map->player.y, map->max_distance * cos(degrees_to_radians(angle)),  map->max_distance * sin(degrees_to_radians(angle))))
              {
                distance = sqrt((interx - map->player.x) * (interx - map->player.x) + (intery - map->player.y) * (intery - map->player.y));
                if(distance < closestdistance)
                {
                  closestdistance = distance;
                  closestx = interx;
                  closesty = intery;
                  face = (unsigned char)ii;
                  last_type = map->entities[k].type;
                  isplayer = 1;
                }
              }


              lastx = pointx[ii];
              lasty = pointy[ii];
            }
            else
            {
              lastx = pointx[0];
              lasty = pointy[0];
              firstx = lastx;
              firsty = lasty;

            if(internal_testRayIntersection(&interx, &intery, lastx, lasty, pointx[3], pointy[3], map->player.x, map->player.y, map->max_distance * cos(degrees_to_radians(angle)),  map->max_distance * sin(degrees_to_radians(angle))))
              {
                distance = sqrt((interx - map->player.x) * (interx - map->player.x) + (intery - map->player.y) * (intery - map->player.y));
                if(distance < closestdistance)
                {
                  closestdistance = distance;
                  closestx = interx;
                  closesty = intery;
                  face = (unsigned char)ii;
                  last_type = map->entities[k].type;
                  isplayer = 1;
                }
              }
            }
          }
        }
      }

      //float linex = (((radians_to_degrees(angle) - map->cam_angle) * map->win_width) / map->cam_fov) + map->win_width/2;
      if(closestdistance != map->max_distance)
      {
        float liney0 = map->win_height/2 - (map->win_height / closestdistance / 4); /* top so it goes negative */
        float liney1 = map->win_height/2 + (map->win_height / closestdistance / 4); /* bottom so it goes positive */
        /*liney0 += map->camera_height_offset * (((map->max_distance - closestdistance)) / map->max_distance); */
        /*liney1 += map->camera_height_offset * (((map->max_distance - closestdistance)) / map->max_distance);*/

        internal_colorCalc(map, closestdistance, face, last_type, isplayer, &r, &g, &b);
        lineRGBA(mainrenderer, j, liney0, j, liney1, r, g, b, 255);
      }
    }
  }


  /* debugging after this */

  if(threadnum = 1 || map->thread_count == 1) /* NOTE: only draw this on a single thread */
  {
    if(map->can_debug) /* TODO do this on one thread */
    {
      for(i = 0; i < map->width * map->height; i++)
      {
        /* second real ray attempt. Testing in the debug window first */

        /* draw the real blocks. First attempt. Cast out (window width) rays and get intersections + distance for fog. Might need to just test for box edges */
        if(map->tiles[i].type != TILE_AIR && map->tiles[i].type != TILE_PLAYERSTART) /* filter out things that shouldnt interferre with the raycast */
        {
          pointx[0] = map->tiles[i].posx;/* top left */
          pointy[0] = map->tiles[i].posy;
          pointx[1] = map->tiles[i].posx + 1.0f;/* top right */
          pointy[1] = map->tiles[i].posy;
          pointx[2] = map->tiles[i].posx + 1.0f;/* bottom right */
          pointy[2] = map->tiles[i].posy + 1.0f;
          pointx[3] = map->tiles[i].posx;/* bottom left */
          pointy[3] = map->tiles[i].posy + 1.0f;

          for(ii = 0; ii < 4; ii++)/* generate the vertecies and faces */
          {
            angle = atan2f(pointy[ii] - map->player.y, pointx[ii] - map->player.x);
            distance = sqrt((pointx[ii] - map->player.x) * (pointx[ii] - map->player.x) + (pointy[ii] - map->player.y) * (pointy[ii] - map->player.y));
            fov0 = map->cam_angle - map->cam_fov/2;
            fov1 = map->cam_angle + map->cam_fov/2;

            //printf("fov1 %f fov2 %f", fov0, fov1);
            if(angle > degrees_to_radians(fov0) && angle < degrees_to_radians(fov1))
            {
              /* do the real raycasting tests. Send out a ray within the fov at the width number of pixels */

              /* temporary line and distance persp test */
              float linex = (((radians_to_degrees(angle) - map->cam_angle) * map->win_width) / map->cam_fov) + map->win_width/2;
              float liney0 = map->win_height/2 - (map->win_height / distance / 4); /* top so it goes negative */
              float liney1 = map->win_height/2 + (map->win_height / distance / 4); /* bottom so it goes positive */
              thickLineRGBA(mainrenderer, linex, liney0, linex, liney1, 4, 255, 255, 255, 255);
              if(map->can_debug)
              {
                /* text at top */
                if(ii == 0)
                  stringRGBA(mainrenderer, linex, liney0, "point 0", 255, 255, 255, 255);
                if(ii == 1)
                  stringRGBA(mainrenderer, linex, liney0, "point 1", 255, 255, 255, 255);
                if(ii == 2)
                  stringRGBA(mainrenderer, linex, liney0, "point 2", 255, 255, 255, 255);
                if(ii == 3)
                  stringRGBA(mainrenderer, linex, liney0, "point 3", 255, 255, 255, 255);
              }
              if(ii != 0) /* skip over 0 because I use last and current NOTE: remember to set a lowest distance variable and only change it if the distance gets closer. might wanna set the vertex too*/
              {
                if(internal_testRayIntersection(&interx, &intery, lastx, lasty, pointx[ii], pointy[ii], map->player.x, map->player.y, map->max_distance * cos(degrees_to_radians(map->cam_angle)),  map->max_distance * sin(degrees_to_radians(map->cam_angle))))
                {
                  distance = sqrt((interx - map->player.x) * (interx - map->player.x) + (intery - map->player.y) * (intery - map->player.y));
                  if(distance < closestdistance)
                  {
                    closestdistance = distance;
                    closestx = interx;
                    closesty = intery;
                  }
                }


                lastx = pointx[ii];
                lasty = pointy[ii];
              }
              else
              {
                lastx = pointx[0];
                lasty = pointy[0];
              }
            }
          }
        }
      }
    }

    /*
    Sint16 xp[] = {20, 60, 30, 50};
    Sint16 yp[] = {100, 200, 200, 100};

    filledPolygonRGBA(mainrenderer, xp, yp, 4, 255, 0, 0, 255);
    */

    //printf("framerate %d\n", SDL_getFramecount(&map->fpsman));


    internal_drawGUI(map);

    if(map->can_debug)
      internal_drawDebug(map, 1);
    else
      internal_drawDebug(map, 0);
    if(map->display_connectbox)
      internal_RenderConnectbox(map);
    /* ================================== */
    SDL_SetRenderDrawColor(mainrenderer, 0, 0, 0, 255);
    /* make sure all threads finished */
    if(map->thread_count > 1 && threadnum == 1)
    {
      int done = 0, i;
      /* only let the first thread control this */
      while(done != 3)/* leaving it to 3 because it's always holding this thread back */
      {
        for(i = 0; i < map->thread_count; i++)
        {

          if(map->threadnum_done[i] == 1)
          {
            printf("%d\n", done);
            map->threadnum_done[i] = 2;
            done++;
          }
        }
      }

      SDL_RenderPresent(mainrenderer);
      SDL_RenderClear(mainrenderer);
    }
    else if(map->thread_count == 1)
    {
      SDL_RenderPresent(mainrenderer);
      SDL_RenderClear(mainrenderer);
    }
    /* ================================== */

    /* for the debug window */
    /* ================================== */
    if(map->can_debug)
    {
      size_t i, ii;
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

      /* draw the entities, player, fov, and direction */
      for(i = 0; i < MAXENTITIES; i++)
      {
        if(map->entities[i].enabled)
        {
          switch(map->entities[i].type)
          {
            case ENTITY_PLAYER_0:
              SDL_SetRenderDrawColor(debugrenderer, 100, 100, 100, 255);
            break;
            case ENTITY_PLAYER_1:
              SDL_SetRenderDrawColor(debugrenderer, 12, 12, 12, 255);
            break;
            case ENTITY_PLAYER_2:
              SDL_SetRenderDrawColor(debugrenderer, 80, 255, 80, 255);
            break;
            default:
              SDL_SetRenderDrawColor(debugrenderer, 23, 6, 160, 255);
          }
          temprect.x = map->entities[i].x  * MAPSCALE - 3;
          temprect.y = map->entities[i].y * MAPSCALE - 3;
          temprect.w = 7;
          temprect.h = 7;
          SDL_RenderFillRect(debugrenderer, &temprect);

          SDL_SetRenderDrawColor(debugrenderer, 255, 70, 10, 255);
          SDL_RenderDrawLine(debugrenderer, map->entities[i].x * MAPSCALE , map->entities[i].y * MAPSCALE , (cos(degrees_to_radians(map->entities[i].angle + 0.0f + map->cam_fov/2)) * FOVRENDERDISTANCE) + map->entities[i].x * MAPSCALE, (sin(degrees_to_radians(map->entities[i].angle + 0.0f + map->cam_fov/2)) * FOVRENDERDISTANCE) + map->entities[i].y * MAPSCALE);
          SDL_RenderDrawLine(debugrenderer, map->entities[i].x * MAPSCALE , map->entities[i].y * MAPSCALE , (cos(degrees_to_radians(map->entities[i].angle + 0.0f - map->cam_fov/2)) * FOVRENDERDISTANCE) + map->entities[i].x * MAPSCALE, (sin(degrees_to_radians(map->entities[i].angle + 0.0f - map->cam_fov/2)) * FOVRENDERDISTANCE) + map->entities[i].y * MAPSCALE);

          SDL_SetRenderDrawColor(debugrenderer, 255, 7, 1, 255);
          SDL_RenderDrawLine(debugrenderer, map->entities[i].x * MAPSCALE , map->entities[i].y * MAPSCALE , (cos(degrees_to_radians(map->entities[i].angle)) * FOVRENDERDISTANCE * 1.3) + map->entities[i].x * MAPSCALE, (sin(degrees_to_radians(map->entities[i].angle)) * FOVRENDERDISTANCE * 1.3) + map->entities[i].y * MAPSCALE);

        }
      }
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
          pointx[0] = map->tiles[i].posx;/* top left */
          pointy[0] = map->tiles[i].posy;
          pointx[1] = map->tiles[i].posx + 1.0f;/* top right */
          pointy[1] = map->tiles[i].posy;
          pointx[2] = map->tiles[i].posx + 1.0f;/* bottom right */
          pointy[2] = map->tiles[i].posy + 1.0f;
          pointx[3] = map->tiles[i].posx;/* bottom left */
          pointy[3] = map->tiles[i].posy + 1.0f;
          /*
          square.p0x = map->tiles[i].posx;
          square.p0y = map->tiles[i].posy;
          square.p1x = map->tiles[i].posx + 1.0f;
          square.p1y = map->tiles[i].posy;
          square.p2x = map->tiles[i].posx + 1.0f;
          square.p2y = map->tiles[i].posy + 1.0f;
          square.p3x = map->tiles[i].posx;
          square.p3y = map->tiles[i].posy + 1.0f;
          */
          for(ii = 0; ii < 4; ii++)/* generate the vertecies and faces */
          {
            angle = atan2f(pointy[ii] - map->player.y, pointx[ii] - map->player.x);
            distance = sqrt((pointx[ii] - map->player.x) * (pointx[ii] - map->player.x) + (pointy[ii] - map->player.y) * (pointy[ii] - map->player.y)) * MAPSCALE;
            fov0 = map->cam_angle - map->cam_fov/2;
            fov1 = map->cam_angle + map->cam_fov/2;

            //printf("fov1 %f fov2 %f", fov0, fov1);
            if(angle > degrees_to_radians(fov0) && angle < degrees_to_radians(fov1))
            {
              SDL_RenderDrawLine(debugrenderer, map->player.x * MAPSCALE , map->player.y * MAPSCALE , (cos(angle) * distance) + map->player.x * MAPSCALE, (sin(angle) * distance) + map->player.y * MAPSCALE);
              //printf("angle %f fov1 %f fov 2 %f facing", radians_to_degrees(angle), fov0, fov1);

            }
            //printf("\n");
          }



        }

      }


      /* ================================== */
      SDL_SetRenderDrawColor(debugrenderer, 0, 0, 0, 0);
      SDL_RenderPresent(debugrenderer);
      SDL_RenderClear(debugrenderer);
    }
    /* ================================== */
  }


}

void changeDebugwinState(unsigned char st)
{
  if(st)
    SDL_ShowWindow(debugwin);
  else
    SDL_HideWindow(debugwin);
}

int raycaster_initbasics(map_settings_t *map)
{
  map->camera_height_offset = 0;
  mainwin = SDL_CreateWindow("rey castor - entry for the gaia raycaster challenge 2018 - 0x3F", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, map->win_width, map->win_height, SDL_WINDOW_SHOWN);
  debugwin = SDL_CreateWindow("map debug window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 550, 550, SDL_WINDOW_HIDDEN);

  mainrenderer = SDL_CreateRenderer(mainwin, -1, SDL_RENDERER_ACCELERATED);
  debugrenderer = SDL_CreateRenderer(debugwin, -1, SDL_RENDERER_ACCELERATED);
  internal_createTexHeader(mainrenderer, &background, space_bmp, space_bmp_len);
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
