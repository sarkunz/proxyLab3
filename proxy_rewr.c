#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "csapp.h"
#include "sbuf.h"
#include "cache.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

//globals
struct sockaddr_in ip4addr;
int sfd;
int done = 0;
cache * head;
int sizeOfCache;

typedef struct entry {
    char* key;
    char* value;
    int size;
    struct entry *next;
} cache;

/////////////////////class code 1////////////////////

typedef struct flow_state_s {
  void (*handler)( int epfd, void *state );
  void *data;
} flow_state_t;

typedef struct listening_data_s {
  int fd;
} listening_data_t;

typedef struct client_connection_reading_request_data_s {
  int cfd;
  char *buffer;
  int capacity;
  int bytes_read;
} ccrrd_t;

typedef struct reading_request_state_data_s {
  int cfd;
  int bytes_read;
  char *buffer;
  int capacity;
} rr_sd_t;

void listening_flow( int epfd, lfd_t *data ) {
  // accept and register new client connections
}

int init_socket(int port){
    //create socket
    ip4addr.sin_family = AF_INET;
    printf("port: %s\n", port);
    ip4addr.sin_port = htons(port);
    ip4addr.sin_addr.s_addr = INADDR_ANY;
    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }
    //bind it
    if (bind(sfd, (struct sockaddr *)&ip4addr, sizeof(struct sockaddr_in)) < 0) {
        close(sfd);
        perror("bind error");
        return -1;
    }
    //listen
    if (listen(sfd, 100) < 0) {
        close(sfd);
        perror("listen error");
        return -1;
    }
	return 0;
}

void init_cache(){
	//signal(SIGINT, sigint_handler);
    head = malloc(sizeof(cache));
	sizeOfCache = 0;
}

int main( int argc, char *argv[] ) {
  int epfd = epoll_create1( 0 );

  int lfd = init_socket( atoi(argv[1]) ); //TODO

  int log_fd = fopen( "logfile", "w" ); //can just log directly rather than caching n stuff

  init_cache(); // no need to guard access now
  
  struct epoll_event evt;

  flow_state_t *ls =  malloc( sizeof( flow_state_t ) );
  listening_data_t *lsd = malloc( sizeof(listening_data_t));

  lsd->fd = lfd;
  
  ls->handler = listening;
  ls->data = lsd;
  
  evt.events = EPOLLIN | EPOLLET;
  evt.data.ptr = ls;
  
  epoll_ctl( epfd, EPOLL_CTL_ADD, lfd, &evt );

  while( 1 ) {
	int n = epoll_wait( epfd, &evt, 1, 1000 );

	if( n == 0 ) {
	  // no events were ready
	  if( done == 1 ) { //global flag
		break;
	  }
	}
	else if( n == -1 ) {
	  // there was an error
	  fprintf(stderr, "Error in while loop \n");
	}
	else {
	  flow_state_t *state = evt.data.ptr;
	  state->handler( epfd, state );
	} 
  }

  return 0;
}

//READ_REQUEST
void reading_request_handler( int epfd, rr_sd_t *data ) {
  rr_sd_t *data = state->data;

  // actually read in a loop until EAGAIN or EWOULDBLOCK
  int bytes_read_this_time = read( data->cfd, buffer + bytes_read, capacity - bytes_read );

	  if( bytes_read_this_time < 0 ) {
		// error
		//if errno is EGAIN or EWOULDBLOCK don't chnage state, cont reading when notified by epoll that there is more data
	  	//else this is an error, cancel req and deregister socket
	  }
	  else if( bytes_read_this_time == 0 ) {
		// connection closed
	  }
	  else {
		data->bytes_read += bytes_read_this_time;
	  }
  
	  if( bytes_read >= 4 && bytes_rnrn( buffer + bytes_read - 4 ) ) {
		// parse request
		// make server connection

		// unregister cfd
		epoll_ctl( epfd, EPOLL_CTL_DEL, cfd, NULL );

		fr_sd_t *next_data = malloc( ... );

		// fill in next_data, some from the rr_sd_t

		free( data );
		
		// set up fr_sd_t with initial data
		// register sfd for writing and
		// associate  with it
		// change state
	  }
}

void sending_request_handler( int epfd, rr_sd_t *data ){
	//loop to write bytes to the server socket until
		//you have written entire req
		
}

// int read_request( int cfd, char *buffer, int capacity ) {
//   int bytes_read = 0;

//   do {
// 	int bytes_read_this_time = read( cfd, buffer + bytes_read, capacity - bytes_read );

// 	if( bytes_read_this_time < 0 ) {
// 	  // error
// 	}
// 	else if( bytes_read_this_time == 0 ) {
// 	  // connection closed
// 	}
// 	else {
// 	  bytes_read += bytes_read_this_time;
// 	}
//   }
//   while( bytes_read < 4 || ! bytes_rnrn( buffer + bytes_read - 4 ) );

//   return bytes_read;
// }

//////////////// class code 2 //////////////////////////////////






void listening( int epfd, flow_state_t *self ) {
  listening_data_t *data = self->data;
  int lfd = data->fd;

  int cfd = accept( lfd );

  epoll_event evt;

  flow_state_t *cc_flow_state = malloc( sizeof( flow_state_t ) );

  cc_flow_state->handler = client_connection_reading_request;

  ccrrd_t *cc_data = malloc(...);

  cc_data->cfd = cfd;
  cc_data->buffer = malloc( 4096 );
  cc_data->capacity = 4096;
  cc_data->bytes_read = 0;

  cc_flow_state->data = cc_data;
  
  evt.events = EPOLLIN | EPOLLET;
  evt.data.ptr = cc_flow_state;
  
  epoll_ctl( epfd, EPOLL_CTL_ADD, cfd, &evt );
}

// client connection handling
// - read request *
// - parse request
// - log request
// - check cache
//   - forward request *
//   - read response *
//   - forward response *
//   OR
//   - forward response *


void client_connection_reading_request( int epfd, flow_state_t *self ) {
  // prelude
  
  int bytes_read_this_time = read( cfd, buffer + bytes_read, capacity - bytes_read );

  if( bytes_read_this_time <= 0 ) {
	// abort like this:
	epoll_ctl( epfd, EPOLL_CTL_DEL, cfd, NULL );
	free( self );
  }
  else {
	bytes_read += bytes_read_this_time;

	if( bytes_read >= 4 && rnrn( buffer + bytes_read - 4 ) ) {
	  parse_request();
	  log_request();

	  if( request_is_in_cache() ) {
	// transition to forwarding response state
	  }
	  else {
	int sfd = make_server_connection();

	epoll_ctl( epfd, EPOLL_CTL_DEL, cfd, NULL );
	
	// build and populate event

	epoll_ctl( epfd, EPOLL_CTL_ADD, sfd, event );
	  }
	}
	else {
	  data->bytes_read = bytes_read;
	}
  }
}
