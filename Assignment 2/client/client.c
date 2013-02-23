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

char * clientMap = NULL;
int playing = 0;
char whoami = 0;

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

void quit() // write error message and quit
{
    fprintf(stderr, "memory exhausted\n");
    exit(1);
}

//returns the size of a character array using a pointer to the first element of the character array
int size(char *ptr)
{
    //variable used to access the subsequent array elements.
    int offset = 0;
    //variable that counts the number of elements in your array
    int count = 0;

    //While loop that tests whether the end of the array has been reached
    while (*(ptr + offset) != '\0')
    {
        //increment the count variable
        ++count;
        //advance to the next element of the array
        ++offset;
    }
    //return the size of the array
    return count;
}

char * specialPrompt(int menu)
{
	int max = 20;
	int c ;
    char* cmdInputs = (char*)malloc(max); // allocate buffer
    if (cmdInputs == 0) quit();
	
	if(whoami == 'X') 
	{
        fprintf(stderr, "\nX> " );
	 playing = 1;
	}
	else if(whoami == 'O') 
	{
        fprintf(stderr, "\nO> " );
	 playing = 1;
	}
	else 
	{
	    fprintf(stderr, "\n?> " );
	 whoami = 0;
		playing = 0;
	}
	
    while (1) { // skip leading whitespace
        c = getchar();
if (playing == 1)
{
if(c == '\n')
{
    pthread_mutex_lock(&gameMap_clientVersion_mutex);
	map(&gameMap_clientCopy[0]);      
    pthread_mutex_unlock(&gameMap_clientVersion_mutex);

	if(whoami == 'X') 
	{
        fprintf(stderr, "\nX> " );
	 playing = 1;
	}
	else if(whoami == 'O') 
	{
        fprintf(stderr, "\nO> " );
	 playing = 1;
	}
	else 
	{
	    fprintf(stderr, "\n?> " );
	 whoami = 0;
		playing = 0;
	}
	
}
}
else 
	    fprintf(stderr, "\n?> " );

         if (c == EOF) break; // end of file
        if (!isspace(c)) {
             ungetc(c, stdin);
             break;
        }
    }

    int i = 0;
    while (1) {
        c = getchar();
        if (isspace(c) || c == EOF) // at end, add terminating zero
		{
            cmdInputs[i] = 0;
            break;
        }
        cmdInputs[i] = c;
        if (i==max-1) { // buffer full
            max = max+max;
            cmdInputs = (char*)realloc(cmdInputs,max); // get a new and larger buffer
            if (cmdInputs == 0) quit();
        }
        i++;
		}
		
		 //printf("The command is %s\n", cmdInputs);
		 
		 return cmdInputs;
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
//printf("whoamii - %d, playing - %d\n", whoami, playing);
	char winner = *str;

		if (winner  == 'X')
		{
			fprintf(stderr, "\nX is the winner.\n");
			if (whoami == 'X') fprintf(stderr, "\nYou win! \n");
			if (whoami == 'O') fprintf(stderr, "\nYou lose! \n");
str++;
        fprintf(stderr,"\n%s\n", str);
			playing = 2; // done playing
		}
		else if (winner == 'O')
		{
			fprintf(stderr, "\nO is the winner.\n");
			if (whoami == 'X') fprintf(stderr, "\nYou lose! \n");
			if (whoami == 'O') fprintf(stderr, "\nYou win! \n");
str++;
        fprintf(stderr,"\n%s\n", str);
			playing = 2; // done playing
		}
		else if (winner == 'D')
		{
			fprintf(stderr,"\nIt's a Draw.\n");
        fprintf(stderr,"\n%s\n", str);
str++;
			playing = 2; // done playing
		}
		else if (winner == 'T')
		{
			fprintf(stderr,"\nX Quits.\n");
			if (whoami == 'X') fprintf(stderr, "\n You quit - You lose! \n");
			if (whoami == 'O') fprintf(stderr, "\n Other side quit - You win! \n");
str++;
        fprintf(stderr,"\n%s\n", str);
			playing = 2; // done playing
		}
		else if (winner == 'U')
		{
			fprintf(stderr, "\nO Quits.\n");
			if (whoami == 'X') fprintf(stderr, "\n Other side quits - You win! \n");
			if (whoami == 'O') fprintf(stderr, "\n You  quit - You lose! \n");
str++;
        fprintf(stderr,"\n%s\n", str);
			playing = 2; // done playing
		}
		else
		{
		}

		str++;

//printf("whoamii - %d, playing - %d\n", whoami, playing);
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

  if (proto_debug())
  fprintf(stderr, "%s: do something %p\n", __func__, s);

  return 1;
}


int 
doRPCCmd(Client *C, char c) 
{
  if (proto_debug())
printf("doRpCCmd open1\n");
  
int rc=-1;

  switch (c) {
  case 'h':  
    {
      rc = proto_client_hello(C->ph);
  if (proto_debug())
      printf("hello: rc=%x\n", rc);
      if ( rc == 1 ) 
      { 
         printf("Connected to <%s:%d>: You are X’s\n", globals.host, globals.port);// get port from globals
         pthread_mutex_lock(&gameMap_clientVersion_mutex);
         whoami = 'X';
	 playing = 1;
         pthread_mutex_unlock(&gameMap_clientVersion_mutex);
      }
      if ( rc == 2 ) 
      { 
         printf("Connected to <%s:%d>: You are Y’s\n", globals.host, globals.port);// get port from globals
         pthread_mutex_lock(&gameMap_clientVersion_mutex);
         whoami = 'O';
	 playing = 1;
         pthread_mutex_unlock(&gameMap_clientVersion_mutex);
      }
      if ( rc == 3 ) printf("Not able to connect to <%s:%d>\n", globals.host, globals.port);// get port from globals
          pthread_mutex_lock(&gameMap_clientVersion_mutex);
	map(&gameMap_clientCopy[0]);      
    pthread_mutex_unlock(&gameMap_clientVersion_mutex);

       if (rc > 0) game_process_reply(C);
    }
    break;
  case 'm':
    scanf("%c", &c);
    rc = proto_client_move(C->ph, c);
    break;
  case '1':
    rc = proto_client_move(C->ph, '1');
    break;
  case '2':
    rc = proto_client_move(C->ph, '2');
    break;
  case '3':
    rc = proto_client_move(C->ph, '3');
    break;
  case '4':
    rc = proto_client_move(C->ph, '4');
    break;
  case '5':
    rc = proto_client_move(C->ph, '5');
    break;
  case '6':
    rc = proto_client_move(C->ph, '6');
    break;
  case '7':
    rc = proto_client_move(C->ph, '7');
    break;
  case '8':
    rc = proto_client_move(C->ph, '8');
    break;
  case '9':
    rc = proto_client_move(C->ph, '9');
    break;
  case 'g':
    rc = proto_client_goodbye(C->ph);
    break;
  default:
    printf("%s: unknown command %c\n", __func__, c);
  }
  // NULL MT OVERRIDE ;-)
  if (proto_debug())
  printf("%s: rc=0x%x\n", __func__, rc);
  if (rc == 0xdeadbeef) rc=1;
  
  if (proto_debug())
printf("doRpCCmd open2\n");
  return rc;
}  

int
doRPC(Client *C)
{

  if (proto_debug())
printf("doRpC open1\n");
  int rc;
  char c;

  //printf("enter (h|m<c>|g): ");
  scanf("%c", &c);
  rc=doRPCCmd(C,c);

  if (proto_debug())
  printf("doRPC: rc=0x%x\n", rc);

  // add TicTacToe message printing here ?

  if (proto_debug())
printf("doRpC open2\n");
  return rc;
}

int 
docmd(Client *C, char cmd)
{
  int rc = 1;

  if (proto_debug())
printf("docmd open1\n");
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
  case '1':
    rc=doRPCCmd(C,'1');
    break;
  case '2':
    rc=doRPCCmd(C,'2');
    break;
  case '3':
    rc=doRPCCmd(C,'3');
    break;
  case '4':
    rc=doRPCCmd(C,'4');
    break;
  case '5':
    rc=doRPCCmd(C,'5');
    break;
  case '6':
    rc=doRPCCmd(C,'6');
    break;
  case '7':
    rc=doRPCCmd(C,'7');
    break;
  case '8':
    rc=doRPCCmd(C,'8');
    break;
  case '9':
    rc=doRPCCmd(C,'9');
    break;
  case 'q':
    doRPCCmd(C,'g');
    rc=-1;
    break;
  case '\n':
  if (proto_debug())
   printf("CALLED\n"); 
   map(clientMap);
    rc=1;
    break;
  default:
    printf("Unkown Command\n");
  }

  if (proto_debug())
printf("doCmd open2\n");
  return rc;
}

int doCMDS(Client *C, char * cmdInput)
{

  if (proto_debug())
printf("doCMDS open1\n");
int rc =1;

if ( strcmp(cmdInput, "connect") == 0 )
{
 rc = doRPCCmd(C,'h');
 return rc;
} 

else if ( strcmp(cmdInput, "disconnect") == 0 )
{
 rc=doRPCCmd(C,'g');
 whoami = '?';
 return rc;
}
/*
if ( strcmp(cmdInput, '\n') == 0 )
{
 map(clientMap);
 return rc;
}

if (strcmp(cmdInput, "1") == 0)
{
 printf("\n Function call\n");

   rc=doRPCCmd(C,'1');
    printf("\nThe value of rc %d\n", rc);

   return rc;
}
if (strcmp(cmdInput, "2") == 0)
{
   rc=doRPCCmd(C,'2');
   return rc;
}
if (strcmp(cmdInput, "3") == 0)
{
   rc=doRPCCmd(C,'3');
   return rc;
}
if (strcmp(cmdInput, "4") == 0)
{
   rc=doRPCCmd(C,'4');
   return rc;
}
if (strcmp(cmdInput, "5") == 0)
{
   rc=doRPCCmd(C,'5');
   return rc;
}
if (strcmp(cmdInput, "6") == 0)
{
   rc=doRPCCmd(C,'6');
   return rc;
}
if (strcmp(cmdInput, "7") == 0)
{
   rc=doRPCCmd(C,'7');
   return rc;
}
if (strcmp(cmdInput, "8") == 0)
{
   rc=doRPCCmd(C,'8');
   return rc;
}
if (strcmp(cmdInput, "9") == 0)
{
   rc=doRPCCmd(C,'9');
   return rc;
}
*/
else if (strcmp(cmdInput, "where") == 0)
{
   //printf("Connected to <ip:port>: You are X’s\n");// get port from globals
   printf("Connected to <%s:%d>.\n", globals.host, globals.port);// get port from globals
   return rc;
}
else if (strcmp(cmdInput, "quit") == 0)
{
   rc = docmd(C,'q');
   return rc;
}
else if (*cmdInput == '\n')
{
  if (proto_debug())
printf("\nDRINKS!!\n");
map(clientMap);
return rc;
}
else
{
//printf("\n%d\n", cmdInput[0]);
  rc = docmd(C,cmdInput[0]); 
  fprintf(stderr, "\nUnkown Command\n");
  return rc;
}
//0-9

//where

//quit

  if (proto_debug())
printf("doCMD open2\n");
return rc;
}

void *
shell(void *arg)
{
  Client *C = arg;
  char c;
  int rc;
  int menu=1;
  char * longcommand;

  while (1) {
    //if ((longcommand=prompt(menu))!=0) rc=doCMDS(C, longcommand);
    //if ((c=prompt(menu))!=0) rc=docmd(C, c);
	(longcommand=specialPrompt(menu)); 
	if (size(longcommand) > 1)
	{
		rc=doCMDS(C, longcommand);
	}
	else 
	{
	 c = longcommand[0];
	 if (c != 0)
		rc=docmd(C, c);
	}

    //if ((longcommand=specialPrompt(menu)) ) rc=doCMDS(C, longcommand);
    //if ((c=prompt(menu))!=0) rc=docmd(C, c);
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

