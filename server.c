#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <float.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>

#define MAXSERVERPACKET 2048
#define MAXENTITIES 100
#define MAXEVENTS 1000

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
  entity_t entity;
  TCPsocket *socket;
  Uint64 id;
  Uint64 lastev;
  unsigned char player_connected;
} player_t;

typedef struct
{
  Uint64 id;
  Uint8 *data;
  Uint64 dlen;
} event_t;

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

TCPsocket server, *client;
IPaddress ip, *clientip;
unsigned int clIp;
Uint64 idcount = 0;
Uint64 eventidcount = 0;
SDL_mutex *idmutx;
player_t players[MAXENTITIES];
event_t events[MAXEVENTS];

/* TODO make a preallocated array of max projectiles and structs because im lazy */

void init()
{
  SDLNet_Init();
  printf("server initializing\n");

  if(SDLNet_ResolveHost(&ip, NULL, 19191) == -1)
  {
    printf("port taken\n");
  }
  server = SDLNet_TCP_Open(&ip);

  idmutx = SDL_CreateMutex();
}

int threadPlayersSend(void *id)
{
  Uint64 lasteventid = eventidcount;
  Uint64 id = *id;


  /*wait for the recv thread to do the initial work. Too lazy to make them both share the init sequence */
  while(mhm)
  {
    while(SDL_LockMutex(idmutx) != 0)
    {
      printf("trying to unlock mutex\n");
    }

    Uint8 *tempec = malloc(1 + sizeof(Uint32));
    tempec[0] = FLAG_GET_EVENTCOUNT;
    tempec[1] = eventidcount - lasteventid;

    printf("sending\n");
    if(SDLNet_TCP_Send(*socket, tempec, 1 + sizeof(Uint32)) < 1 + sizeof(Uint32))
      break;

    Uint64 i;
    for(i = lasteventid; i < eventidcount; i++)
    {
      printf("sending %s\n", &events[i % MAXEVENTS].data[1]);
      if(SDLNet_TCP_Send(*socket, events[i % MAXEVENTS].data, events[i % MAXEVENTS].dlen) < events[i % MAXEVENTS].dlen)
        break;
    }

    lasteventid = eventidcount;


    SDL_UnlockMutex(idmutx);
  }

  SDL_Delay(20);
}

int threadPlayersRecv(void *id)
{
  Uint64 id = *id; /* set this */


  Uint8 cdata[MAXSERVERPACKET];
  Uint64 cdatalen = 0;


  Uint8 *data; /* temp stuff */
  Uint64 data_len = 0;

  do
  {
    printf("trying to initially unlock mutex\n");
  }
  while(SDL_LockMutex(idmutx) != 0);
  /* ====================== */

  /* send the players id first*/
  Uint64 serialized_data = 5, dlen = 0; /* yeah, I gave up checking validity */ //sizeof(Uint8) + 10 + sizeof(Uint8) + (sizeof(Uint16) + strlen(px_str) + 1) + (sizeof(Uint16) + strlen(py_str) + 1) + (sizeof(Uint16) + strlen(pa_str) + 1) + sizeof(Uint8), dlen = 0; /* messageheader name, type, position(x,y), angle, and health NOTE yeah, I know its not network byte order */
  id = idcount;
  data = calloc(1, sizeof(Uint64) + 1);

  data[0] = FLAG_SERVER_RETURN_ID;
  data[1] = id;


  /* TODO check and make sure theres not too many players before adding player and recycle old ids */
  printf("sending\n");
  SDLNet_TCP_Send(*socket, data, sizeof(Uint64) + 1);


  idcount++;


  /* start the send thread because the client connected */

  if(SDL_CreateThread(threadPlayersSend, "player_send", &id) == NULL)
  {
    printf"thread error\n");
    return 1;
  }
  /* ====================== */
  SDL_UnlockMutex(idmutx);

  /* do the real work loop and only unlock the mutex when needed */

  while(client_connected)
  {
    printf("recieving\n");
    if((dlen = SDLNet_TCP_Recv(*socket, cdata, MAXSERVERPACKET)) <= 0)
    {
      printf("client disconnect\n");
      break;
    }

    switch(cdata[0])
    {
      case FLAG_RETURN_DETAILS:/* the client sent data about its characteristics */
      /* check and fix all data the client sends */
      if(dlen > serialized_data)
      {
        cdata[sizeof(Uint8) + 9] = 0x00; /* just in case no null terminator is present */
        if(entities[id].name != NULL)
          free(entities[id].name);

        entities[id].name = SDL_strdup(&cdata[sizeof(Uint8)]);
        entities[id].type = cdata[sizeof(Uint8) + 10];
        /* TODO make sure the end of each string is null terminated */
        entities[id].x = atof(&cdata[sizeof(Uint8) + 10 + sizeof(Uint8) + sizeof(Uint16)]);
        entities[id].y = atof(&cdata[sizeof(Uint8) + 10 + sizeof(Uint8) + sizeof(Uint16) + ( (Uint16)cdata[sizeof(Uint8) + 10 + sizeof(Uint8)] ) + sizeof(Uint16)]);
        entities[id].angle = atof(&cdata[sizeof(Uint8) + 10 + sizeof(Uint8) + sizeof(Uint16) + ( (Uint16)cdata[sizeof(Uint8) + 10 + sizeof(Uint8)] ) + sizeof(Uint16) + ((Uint16)cdata[sizeof(Uint8) + 10 + sizeof(Uint8) + sizeof(Uint16) + ( (Uint16)cdata[sizeof(Uint8) + 10 + sizeof(Uint8)] )]) + sizeof(Uint16)]);
        entities[id].health = cdata[dlen - sizeof(Uint8)];

        /* TODO check for colliding players? nah, i'll just check that clientside */


        while(SDL_LockMutex(idmutx) != 0)
        {
          printf("trying to unlock mutex\n");
        }
        printf("adding event %lu\n", eventidcount % MAXEVENTS);
        if(events[eventidcount % MAXEVENTS].data == NULL)
          free(events[eventidcount % MAXEVENTS].data);
        events[eventidcount % MAXEVENTS].data = calloc(1, dlen);
        memcpy(events[eventidcount % MAXEVENTS].data, cdata, dlen);
        events[eventidcount % MAXEVENTS].data[0] = (Uint8)FLAG_GET_ID_DETAILS; /* change it to the other type */
        events[eventidcount % MAXEVENTS].dlen = dlen;
        events[eventidcount % MAXEVENTS].id = eventidcount;
        eventidcount++;
        SDL_UnlockMutex(idmutx);


        printf("client name %s of %d type moved to %f %f and is facing %f, they look pretty healthy at %d\n", entities[id].name, entities[id].type, entities[id].x, entities[id].y, entities[id].angle, entities[id].health);
      }
      else
        printf("data not correct\n");

      break;
      /* =============================== */
      default:
        printf("illegal flag header\n");
    }



    SDL_Delay(20);
  }
  printf("destroying client thread\n");
  SDLNet_TCP_Close(sock);
  return 0;
}

int main(int argc, char **argv)
{
  SDL_Thread *temp;
  init();
  printf("ready\n");

  while(1)
  {
    /* accept and make player threads */
    client = calloc(1, sizeof(TCPsocket));
    if(client == NULL)
      printf("mem error\n");

    *client = SDLNet_TCP_Accept(server);
    if(*client != NULL)
    {
      clientip = SDLNet_TCP_GetPeerAddress(*client); /* ip printing credit https://gist.github.com/psqq/b92243f2149fcf4dd46370d4c0b5fef9 */

      clIp = SDL_SwapBE32(clientip->host);

      printf("Client [%d.%d.%d.%d] connected\n", clIp >> 24, (clIp >> 16) & 0xFF, (clIp >> 8) & 0xFF, clIp & 0xFF);

      if(*client != NULL)
      {
        players[idcount].id = idcount;
        players[idcount].socket = client;
        temp = SDL_CreateThread(threadPlayersRecv, "player_recv", &players[idcount].id);
        idcount++;

        if(temp == NULL)
          printf("threading error\n");
      }
    }

    SDL_Delay(50);
  }
  return 0;
}
