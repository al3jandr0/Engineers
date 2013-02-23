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
#include <sys/types.h>
#include <strings.h>
#include <errno.h>
#include <pthread.h>

#include "net.h"
#include "protocol.h"
#include "protocol_utils.h"
#include "protocol_server.h"
#include "./../lib/TicTacToe.h"

#define PROTO_SERVER_MAX_EVENT_SUBSCRIBERS 1024

static
char *gameReplyMsg[] = { "Not your turn yet!\n",  // 0
                         "Not a valid move!\n",   // 1
                         NULL,                    // 2
                         NULL,                    // 3
                         NULL,                    // 4
                         NULL };                  // 5

pthread_mutex_t game_mutex;
pthread_mutex_t gameMapVersion_mutex;
Proto_StateVersion gameMapVersion;

struct {
    FDType   RPCListenFD;
    PortType RPCPort;
    
    
    FDType             EventListenFD;
    PortType           EventPort;
    pthread_t          EventListenTid;
    pthread_mutex_t    EventSubscribersLock;
    int                EventLastSubscriber;
    int                EventNumSubscribers;
    FDType             EventSubscribers[PROTO_SERVER_MAX_EVENT_SUBSCRIBERS];
    Proto_Session      EventSession;
    pthread_t          RPCListenTid;
    Proto_MT_Handler   session_lost_handler;
    Proto_MT_Handler   base_req_handlers[PROTO_MT_REQ_BASE_RESERVED_LAST -
                                         PROTO_MT_REQ_BASE_RESERVED_FIRST-1];
} Proto_Server;

extern PortType proto_server_rpcport(void) { return Proto_Server.RPCPort; }
extern PortType proto_server_eventport(void) { return Proto_Server.EventPort; }
extern Proto_Session *
proto_server_event_session(void)
{
    return &Proto_Server.EventSession;
}

extern int
proto_server_set_session_lost_handler(Proto_MT_Handler h)
{
    Proto_Server.session_lost_handler = h;
}

extern int
proto_server_set_req_handler(Proto_Msg_Types mt, Proto_MT_Handler h)
{
    int i;
    
    if (mt>PROTO_MT_REQ_BASE_RESERVED_FIRST &&
        mt<PROTO_MT_REQ_BASE_RESERVED_LAST) {
        i = mt - PROTO_MT_REQ_BASE_RESERVED_FIRST - 1;
        Proto_Server.base_req_handlers[i]=h;
        return 1;
    } else {
        return -1;
    }
}


static int
proto_server_record_event_subscriber(int fd, int *num)
{
    int rc=-1;
    
    pthread_mutex_lock(&Proto_Server.EventSubscribersLock);
    
    if (Proto_Server.EventLastSubscriber < PROTO_SERVER_MAX_EVENT_SUBSCRIBERS
        && Proto_Server.EventSubscribers[Proto_Server.EventLastSubscriber]
        ==-1) {
        Proto_Server.EventSubscribers[Proto_Server.EventLastSubscriber]=fd;
        *num=Proto_Server.EventLastSubscriber;
        Proto_Server.EventLastSubscriber++;
        Proto_Server.EventNumSubscribers++;
        rc = 1;
    } else {
        int i;
        for (i=0; i< PROTO_SERVER_MAX_EVENT_SUBSCRIBERS; i++) {
            if (Proto_Server.EventSubscribers[i]==-1) {
                Proto_Server.EventSubscribers[i]=fd;
                Proto_Server.EventNumSubscribers++;
                *num=i;
                rc=1;
            }
        }
    }
    
    pthread_mutex_unlock(&Proto_Server.EventSubscribersLock);
    
    return rc;
}

static
void *
proto_server_event_listen(void *arg)
{
    int fd = Proto_Server.EventListenFD;
    int connfd;
    
    if (net_listen(fd)<0) {
        exit(-1);
    }
    
    for (;;) {
        connfd = net_accept(fd);
        if (connfd < 0) {
            fprintf(stderr, "Error: EventListen accept failed (%d)\n", errno);
        } else {
            int i;
            fprintf(stderr, "EventListen: connfd=%d -> ", connfd);
            
            if (proto_server_record_event_subscriber(connfd,&i)<0) {
                fprintf(stderr, "oops no space for any more event subscribers\n");
                close(connfd);
            } else {
                fprintf(stderr, "subscriber num %d\n", i);
            }
        }
    }
}

void
proto_server_post_event(void)
{
    int i;
    int num;
    
    pthread_mutex_lock(&Proto_Server.EventSubscribersLock);
    
    i = 0;
    num = Proto_Server.EventNumSubscribers;
    while (num > 0) {
        Proto_Server.EventSession.fd = Proto_Server.EventSubscribers[i];
        if (Proto_Server.EventSession.fd != -1) {
            num--;
            if (proto_session_send_msg(&Proto_Server.EventSession,0)<0) {
                // must have lost an event connection
                close(Proto_Server.EventSession.fd);
                Proto_Server.EventSubscribers[i]=-1;
                Proto_Server.EventNumSubscribers--;
                Proto_Server.session_lost_handler(&Proto_Server.EventSession);
            }
            // FIXME: add ack message here to ensure that game is updated
            // correctly everywhere... at the risk of making server dependent
            // on client behaviour  (use time out to limit impact... drop
            // clients that misbehave but be carefull of introducing deadlocks
        }
        i++;
    }
    proto_session_reset_send(&Proto_Server.EventSession);
    pthread_mutex_unlock(&Proto_Server.EventSubscribersLock);
}

// Broad casts the current map to Subscribers
int
doUpdateClientsGame(int updateMapVersion)
{
  Proto_Session *s;
  Proto_Msg_Hdr hdr;
  char mapBuffer[PROTO_SESSION_BUF_SIZE-1];

  if (proto_debug())
  fprintf(stderr, "doUpdateClientsGame called\n");  // DEBUG 

  bzero(&mapBuffer[0], sizeof(mapBuffer));
  bzero(&hdr, sizeof(hdr));

  s = proto_server_event_session();
  // set sver if nescesary
  hdr.type = PROTO_MT_EVENT_BASE_UPDATE;
  pthread_mutex_lock(&gameMapVersion_mutex);
  if (updateMapVersion)
     gameMapVersion.raw++;
  hdr.sver.raw = gameMapVersion.raw;
  pthread_mutex_unlock(&gameMapVersion_mutex); 
  proto_session_hdr_marshall(s, &hdr);

  game(&mapBuffer[0]);

  if (proto_debug())
  fprintf(stderr, "doUpdateClientsGame: mapBuffer\n%s\n", mapBuffer); 

  if (proto_session_body_marshall_bytes(s, sizeof(mapBuffer), &mapBuffer[0]) < 0)
     fprintf(stderr, "doUpdateClientsGame: proto_session_body_marshall_bytes failed\n"); 
  proto_server_post_event();
  return 1;
}


static void *
proto_server_req_dispatcher(void * arg)
{
    Proto_Session s;
    Proto_Msg_Types mt;
    Proto_MT_Handler hdlr;
    int i;
    unsigned long arg_value = (unsigned long) arg;
    
    pthread_detach(pthread_self());
    
    proto_session_init(&s);
    
    s.fd = (FDType) arg_value;
    
    fprintf(stderr, "proto_rpc_dispatcher: %p: Started: fd=%d\n",
            pthread_self(), s.fd);
    
    for (;;) {
        if (proto_session_rcv_msg(&s)==1) {
            mt = proto_session_hdr_unmarshall_type(&s);
            if (mt > PROTO_MT_REQ_BASE_RESERVED_FIRST &&
                mt < PROTO_MT_REQ_BASE_RESERVED_LAST) {
                i = mt - PROTO_MT_REQ_BASE_RESERVED_FIRST - 1;
                hdlr = Proto_Server.base_req_handlers[i];
                if (hdlr(&s)<0) goto leave;
            }
        } else {
            goto leave;
        }
    }
leave:
    Proto_Server.session_lost_handler(&s);
    close(s.fd);
    return NULL;
}

static
void *
proto_server_rpc_listen(void *arg)
{
    int fd = Proto_Server.RPCListenFD;
    unsigned long connfd;
    pthread_t tid;
    
    if (net_listen(fd) < 0) {
        fprintf(stderr, "Error: proto_server_rpc_listen listen failed (%d)\n", errno);
        exit(-1);
    }
    
    for (;;) {
        connfd = net_accept(fd);
        if (connfd < 0) {
            fprintf(stderr, "Error: proto_server_rpc_listen accept failed (%d)\n", errno);
        } else {
            pthread_create(&tid, NULL, &proto_server_req_dispatcher,
                           (void *)connfd);
        }
    }
}

extern int
proto_server_start_rpc_loop(void)
{
    if (pthread_create(&(Proto_Server.RPCListenTid), NULL,
                       &proto_server_rpc_listen, NULL) !=0) {
        fprintf(stderr,
                "proto_server_rpc_listen: pthread_create: create RPCListen thread failed\n");
        perror("pthread_create:");
        return -3;
    }
    return 1;
}

static int
proto_session_lost_default_handler(Proto_Session *s)
{
    fprintf(stderr, "Session lost...:\n");
    proto_session_dump(s);
    return -1;
}

static int
proto_server_mt_null_handler(Proto_Session *s)
{
    int rc=1;
    Proto_Msg_Hdr h;
    
    fprintf(stderr, "proto_server_mt_null_handler: invoked for session:\n");
    proto_session_dump(s);
    
    // setup dummy reply header : set correct reply message type and
    // everything else empty
    bzero(&h, sizeof(h));
    h.type = proto_session_hdr_unmarshall_type(s);
    h.type += PROTO_MT_REP_BASE_RESERVED_FIRST;
    proto_session_hdr_marshall(s, &h);
    
    // setup a dummy body that just has a return code
    proto_session_body_marshall_int(s, 0xdeadbeef);
    
    rc=proto_session_send_msg(s,1);
    
    return rc;
}

static int
proto_server_mt_join_game_handler(Proto_Session *s)
{
    int rc=1;
    Proto_Msg_Hdr h;
    int player;
  
    if (proto_debug()) 
       fprintf(stderr, "proto_server_mt_join_game_handler: invoked for session:\n");
    //proto_session_dump(s);
   
    // TicTacToe game add a player and return a either 1 or 2 or -1.
    // If the playyer cant be added, return -1
    // X = 1, Y = 2 
    // int addPlayer(int fd);
    pthread_mutex_lock(&game_mutex);
    player = addPlayer(s->fd); 
    pthread_mutex_unlock(&game_mutex);
    if (proto_debug()) 
    fprintf(stderr, "%d  addPlayer() = %d\n", s->fd, player); // DEBUGING
    
    // TODO: versioning. sver
    // proto_session_hdr_marshall_sver(s, v);
    bzero(&h, sizeof(s));
    h.type = proto_session_hdr_unmarshall_type(s);
    h.type += PROTO_MT_REP_BASE_RESERVED_FIRST;
    proto_session_hdr_marshall(s, &h);
    
    proto_session_body_marshall_int(s, player); 
    rc=proto_session_send_msg(s,1);
   
    doUpdateClientsGame(0);

    return rc;
}

static int
proto_server_mt_move_handler(Proto_Session *s)
{
    int rc=1;
    Proto_Msg_Hdr h;
    char position;
    char reply[PROTO_SESSION_BUF_SIZE-1];
    int TicTac, intchar;

    if (proto_debug()) 
    fprintf(stderr, "proto_server_mt_move_handler: invoked for session:\n");
    //proto_session_dump(s);

    // read msg here
    proto_session_body_unmarshall_char(s, 0, &position);

    // call TicTacToe function. This function should return an int which represents the following 
    /* 0  “Not your turn yet!”
     * 1  “Not a valid move!”  
     */
    // int func( int fd, char position ) 
    intchar = position - '0';
    pthread_mutex_lock(&game_mutex);
    TicTac = logic( s->fd, intchar);
    pthread_mutex_unlock(&game_mutex);
    // TODO: if debug func
    if (proto_debug()) 
    {
       fprintf(stderr, "%d  intchar = %d\n", s->fd, intchar); // DEBUGING
       fprintf(stderr, "%d  logic() = %d\n", s->fd, TicTac); // DEBUGING
       fprintf(stderr, "%d  move: %c\n", s->fd, position);   // DEBUGING
    }
    bzero(&h, sizeof(h));
    h.type = proto_session_hdr_unmarshall_type(s);
    h.type += PROTO_MT_REP_BASE_RESERVED_FIRST;
    proto_session_hdr_marshall(s, &h);
    
    // proto_session_body_marshall_int(s, 0x00000002);
    // reply with message from TicTacToe
    bzero(&reply[0], sizeof(reply));
    if (TicTac < 2) 
       strncpy(reply, gameReplyMsg[TicTac], sizeof(reply) - 1 );
    if (proto_session_body_marshall_bytes(s, sizeof(reply), &reply[0]) < 0 )
       fprintf(stderr, "proto_server_mt_move_handler: "
               "proto_session_body_marshall_bytes failed\n");
                                   
    rc=proto_session_send_msg(s,1);

    if ( TicTac > 1 )
       doUpdateClientsGame(1);
    if ( TicTac == 2  ) /* Game is over because of a successfull move*/
    {
        pthread_mutex_lock(&game_mutex);
	resetGame();
        pthread_mutex_unlock(&game_mutex);
        pthread_mutex_lock(&gameMapVersion_mutex);
        gameMapVersion.raw++;
        pthread_mutex_unlock(&gameMapVersion_mutex);
    }
    return rc;
}

static int
proto_server_mt_leave_game_handler(Proto_Session *s)
{
    int rc=1;
    Proto_Msg_Hdr h;
    int qq;    
    
    if (proto_debug())
    fprintf(stderr, "proto_server_mt_leave_game_handler: invoked for session:\n");
    //proto_session_dump(s);
   
    // remove player from TicTacToe Game
    pthread_mutex_lock(&game_mutex);
    qq = removePlayer(s->fd);
    pthread_mutex_unlock(&game_mutex);

    if (proto_debug())
    fprintf(stderr, "%d  quit() = %d\n", s->fd, qq); // DEBUGING

    bzero(&h, sizeof(h));
    h.type = proto_session_hdr_unmarshall_type(s);
    h.type += PROTO_MT_REP_BASE_RESERVED_FIRST;
    proto_session_hdr_marshall(s, &h);
 
    /* quit return values
     * 1 successfully removed player
     * 2 unsuccsesfully removed player. Player wasnt registerred in the game
     */
    
    proto_session_body_marshall_int(s, qq);
    rc=proto_session_send_msg(s,1);
   
    // if rmovePlayer == 1. call update
    if ( qq == 1  )
    {
       doUpdateClientsGame(1);
       resetGame();
       pthread_mutex_lock(&gameMapVersion_mutex);
       gameMapVersion.raw++;
       pthread_mutex_unlock(&gameMapVersion_mutex);
    }
    return rc;
}



extern int
proto_server_init(void)
{
    int i;
    int rc;
    
    proto_session_init(&Proto_Server.EventSession);
    
    proto_server_set_session_lost_handler(
                                          proto_session_lost_default_handler);
    //for (i=PROTO_MT_REQ_BASE_RESERVED_FIRST+1;
    //     i<PROTO_MT_REQ_BASE_RESERVED_LAST; i++) {
        //proto_server_set_req_handler(i, proto_server_mt_null_handler);
        //proto_server_set_req_handler(i, proto_server_mt_join_game_handler);
    //}
    // set_up_actual_game rpc handlers
    proto_server_set_req_handler( PROTO_MT_REQ_BASE_HELLO, proto_server_mt_join_game_handler);   
    proto_server_set_req_handler( PROTO_MT_REQ_BASE_MOVE, proto_server_mt_move_handler);   
    proto_server_set_req_handler( PROTO_MT_REQ_BASE_GOODBYE, proto_server_mt_leave_game_handler);   

    pthread_mutex_lock(&gameMapVersion_mutex); 
    gameMapVersion.raw = 0;
    pthread_mutex_unlock(&gameMapVersion_mutex);   

    for (i=0; i<PROTO_SERVER_MAX_EVENT_SUBSCRIBERS; i++) {
        Proto_Server.EventSubscribers[i]=-1;
    }
    Proto_Server.EventNumSubscribers=0;
    Proto_Server.EventLastSubscriber=0;
    pthread_mutex_init(&Proto_Server.EventSubscribersLock, 0);
    
    
    rc=net_setup_listen_socket(&(Proto_Server.RPCListenFD),
                               &(Proto_Server.RPCPort));
    
    if (rc==0) { 
        fprintf(stderr, "prot_server_init: net_setup_listen_socket: FAILED for RPCPort\n");
        return -1;
    }
    
    Proto_Server.EventPort = Proto_Server.RPCPort + 1;
    
    rc=net_setup_listen_socket(&(Proto_Server.EventListenFD),
                               &(Proto_Server.EventPort));
    
    if (rc==0) { 
        fprintf(stderr, "proto_server_init: net_setup_listen_socket: FAILED for EventPort=%d\n", 
                Proto_Server.EventPort);
        return -2;
    }
    
    if (pthread_create(&(Proto_Server.EventListenTid), NULL, 
                       &proto_server_event_listen, NULL) !=0) {
        fprintf(stderr, 
                "proto_server_init: pthread_create: create EventListen thread failed\n");
        perror("pthread_createt:");
        return -3;
    }
    
    return 0;
}
