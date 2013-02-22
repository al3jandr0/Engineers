/******************************************************************************
* Copyright (C) 2011 by Jonathan Appavoo, Boston University
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lib/types.h"
#include "../lib/protocol_client.h"
#include "../lib/protocol_utils.h"

#define STRLEN 81

char * clientMap;
int playing = 0;
char whoami;

struct Globals {
  char host[STRLEN];
  PortType port;
} globals;


typedef struct ClientState  {
  int data;
  Proto_Client_Handle ph;
} Client;

static int
clientInit(Client *C)
{
  bzero(C, sizeof(Client));

  // initialize the client protocol subsystem
  if (proto_client_init(&(C->ph))<0) {
    fprintf(stderr, "client: main: ERROR initializing proto system\n");
    return -1;
  }
  return 1;
}


static int
update_event_handler(Proto_Session *s)
{
  Client *C = proto_session_get_data(s);

  fprintf(stderr, "%s: called", __func__);
  return 1;
}


int 
startConnection(Client *C, char *host, PortType port, Proto_MT_Handler h)
{
  if (globals.host[0]!=0 && globals.port!=0) {
    if (proto_client_connect(C->ph, host, port)!=0) {
      fprintf(stderr, "failed to connect\n");
      return -1;
    }
    proto_session_set_data(proto_client_event_session(C->ph), C);
#if 0
    if (h != NULL) {
      proto_client_set_event_handler(C->ph, PROTO_MT_EVENT_BASE_UPDATE, 
				     h);
    }
#endif
    return 1;
  }
  return 0;
}


int
prompt(int menu) 
{
  static char MenuString[] = "\n?> ";
  
  if(whoami == 'X') 
        sprintf(MenuString, "\nX> " );
  	//MenuString = "\nX> ";
  
  if(whoami == 'O') 
        sprintf(MenuString, "\nO> " );
  	//MenuString = "\nY> ";
  
  int ret;
  int c=0;

  if (menu) fprintf(stderr, "%s", MenuString);
  fflush(stdout);
  c = getchar();
  return c;
}


void map(char* str)
{
	clientMap = str;
	
	char winner = *str;

		if (winner  == 'X')
		{
			fprintf(stderr, "\nX is the winner.\n");
			if (whoami == 'X') fprintf(stderr, "\nYou win! \n");
			if (whoami == 'O') fprintf(stderr, "\nYou lose! \n");
			playing = 2; // done playing
		}
		else if (winner == 'O')
		{
			fprintf(stderr, "\nO is the winner.\n");
			if (whoami == 'X') fprintf(stderr, "\nYou lose! \n");
			if (whoami == 'O') fprintf(stderr, "\nYou win! \n");
			playing = 2; // done playing
		}
		else if (winner == 'D')
		{
			fprintf(stderr,"\nIt's a Draw.\n");
			playing = 2; // done playing
		}
		else if (winner == 'T')
		{
			fprintf(stderr,"\nX Quits.\n");
			if (whoami == 'X') fprintf(stderr, "\n You quit - You lose! \n");
			if (whoami == 'O') fprintf(stderr, "\n Other side quit - You win! \n");
			playing = 2; // done playing
		}
		else if (winner == 'U')
		{
			fprintf(stderr, "\nO Quits.\n");
			if (whoami == 'X') fprintf(stderr, "\n Other side quits - You win! \n");
			if (whoami == 'O') fprintf(stderr, "\n You  quit - You lose! \n");
			playing = 2; // done playing
		}
		else
		{
			playing = 1; //playing game
		}

		str++;

if (playing == 1)
        fprintf(stderr,"\n%s\n", str);
}


// FIXME:  this is ugly maybe the speration of the proto_client code and
//         the game code is dumb
int
game_process_reply(Client *C)
{
  Proto_Session *s;

  s = proto_client_rpc_session(C->ph);

  fprintf(stderr, "%s: do something %p\n", __func__, s);

  return 1;
}


int 
doRPCCmd(Client *C, char c) 
{
  int rc=-1;

  switch (c) {
  case 'h':  
    {
      rc = proto_client_hello(C->ph);
      printf("hello: rc=%x\n", rc);
      if ( rc == 1 ) { printf("Connected to <%s:%d>: You are X’s\n", globals.host, globals.port);// get port from globals
			whoami = 'X';}
      if ( rc == 2 ) { printf("Connected to <%s:%d>: You are Y’s\n", globals.host, globals.port);// get port from globals
			whoami = 'O';}
      if ( rc == 3 ) printf("Not able to connect to <%s:%d>\n", globals.host, globals.port);// get port from globals
      if (rc > 0) game_process_reply(C);
    }
    break;
  case 'm':
    scanf("%c", &c);
    rc = proto_client_move(C->ph, c);
    break;
  case 'g':
    rc = proto_client_goodbye(C->ph);
    playing = 0;
    break;
  default:
    printf("%s: unknown command %c\n", __func__, c);
  }
  // NULL MT OVERRIDE ;-)
  printf("%s: rc=0x%x\n", __func__, rc);
  if (rc == 0xdeadbeef) rc=1;
  return rc;
}

int
doRPC(Client *C)
{
  int rc;
  char c;

  printf("enter (h|m<c>|g): ");
  scanf("%c", &c);
  rc=doRPCCmd(C,c);

  printf("doRPC: rc=0x%x\n", rc);

  // add TicTacToe message printing here ?

  return rc;
}
/*
int doCMDS(Client *C, char * cmdInput)
{

if ( strcmp(cmdInput, "connect") == 0 )
{
 rc=doRPCCmd(C,'h');
} 

if ( strcmp(cmdInput, "disconnect") == 0 )
{
 rc=doRPCCmd(C,'h');
}

if ( strcmp(cmdInput, '\n') == 0 )
{
 map(clientMap);
}

//0-9

//where

//quit

}
*/
int 
docmd(Client *C, char cmd)
{
  int rc = 1;

  switch (cmd) {
  case 'd':
    proto_debug_on();
    break;
  case 'D':
    proto_debug_off();
    break;
  case 'r':
    rc = doRPC(C);
    break;
  case 'q':
    rc=-1;
    break;
  case '\n':
    rc=1;
    break;
  default:
    printf("Unkown Command\n");
  }
  return rc;
}

void *
shell(void *arg)
{
  Client *C = arg;
  char c;
  int rc;
  int menu=1;

  while (1) {
    if ((c=prompt(menu))!=0) rc=docmd(C, c);
    if (rc<0) break;
    if (rc==1) menu=1; else menu=0;
  }

  fprintf(stderr, "terminating\n");
  fflush(stdout);
  return NULL;
}

void 
usage(char *pgm)
{
  fprintf(stderr, "USAGE: %s <port|<<host port> [shell] [gui]>>\n"
           "  port     : rpc port of a game server if this is only argument\n"
           "             specified then host will default to localhost and\n"
	   "             only the graphical user interface will be started\n"
           "  host port: if both host and port are specifed then the game\n"
	   "examples:\n" 
           " %s 12345 : starts client connecting to localhost:12345\n"
	  " %s localhost 12345 : starts client connecting to locaalhost:12345\n",
	   pgm, pgm, pgm, pgm);
 
}

void
initGlobals(int argc, char **argv)
{
  bzero(&globals, sizeof(globals));

  if (argc==1) {
    usage(argv[0]);
    exit(-1);
  }

  if (argc==2) {
    strncpy(globals.host, "localhost", STRLEN);
    globals.port = atoi(argv[1]);
  }

  if (argc>=3) {
    strncpy(globals.host, argv[1], STRLEN);
    globals.port = atoi(argv[2]);
  }

}

int 
main(int argc, char **argv)
{
  Client c;

  initGlobals(argc, argv);

  if (clientInit(&c) < 0) {
    fprintf(stderr, "ERROR: clientInit failed\n");
    return -1;
  }    

  // ok startup our connection to the server
  if (startConnection(&c, globals.host, globals.port, update_event_handler)<0) {
    fprintf(stderr, "ERROR: startConnection failed\n");
    return -1;
  }

  shell(&c);

  return 0;
}

