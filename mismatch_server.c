/*
    This assignment uses sample code from the following sources:
    * lab 11 buffered server by David.
    * sample solution of Assignment 2 by Gabe.
    * the header file from starter code utils.c ... all other code is deleted.
    Thanks for the support from src providers, and my TA Elarine, and 
    course instructor Marina.
*/

#ifndef ASSIGNMENT4
#define ASSIGNMENT4

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#include "utils.h"

#endif

#ifndef PORT
  #define PORT 53891
#endif

int listenfd;
CNode* connected_clients;

/* Returns 0 when there's a client ready for reading. Also assigns fd.
   Returns 1 when it is ready for accepting.
*/
int read_multiplexer(int *fd_pointer) {
  CNode* curr = connected_clients;
  fd_set fdlist;
  int maxfd = listenfd;
  // Setup fdlist.
  FD_ZERO(&fdlist);
  FD_SET(listenfd, &fdlist);
  // Iterate through all connected clients and add them to fdlist
  while ( curr != NULL) {
    if (maxfd < curr -> fd) {
      maxfd = curr -> fd;
    }
    FD_SET(curr -> fd, &fdlist);
    curr = curr -> next;
  }
  // Select.
  if ( select(maxfd+1, &fdlist, NULL, NULL, NULL) == -1 ) {
    perror("select");
    exit(1);
  }
  if ( FD_ISSET(listenfd, &fdlist) ) {
    // Ready for accept.
    return 1;
  } 
  curr = connected_clients;
  while ( curr != NULL) {
    if ( FD_ISSET(curr -> fd, &fdlist) ) {
      *fd_pointer = curr -> fd;
      break;
    }
    curr = curr -> next;
  }
  return 0;
   
}


int setup(void) {
  int on = 1, status;
  struct sockaddr_in self;
  int listenfd;
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }

  // Make sure we can reuse the port immediately after the
  // server terminates.
  status = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                      (const char *) &on, sizeof(on));
  if(status == -1) {
    perror("setsockopt -- REUSEADDR");
  }

  self.sin_family = AF_INET;
  self.sin_addr.s_addr = INADDR_ANY;
  self.sin_port = htons(PORT);
  memset(&self.sin_zero, 0, sizeof(self.sin_zero));  // Initialize sin_zero to 0

  printf("Listening on %d\n", PORT);

  if (bind(listenfd, (struct sockaddr *)&self, sizeof(self)) == -1) {
    perror("bind"); // probably means port is in use
    exit(1);
  }

  if (listen(listenfd, 5) == -1) {
    perror("listen");
    exit(1);
  }
  return listenfd;
}

/*Search the first inbuf characters of buf for a network newline ("\r\n").
  Return the location of the '\r' if the network newline is found,
  or -1 otherwise.
  Definitely do not use strchr or any other string function in here. (Why not?)

  Answer: Because this string may not be null-terminated correctly!
*/
int find_network_newline(const char *buf, int inbuf) {
  // Step 1: write this function
  for (int i = 0; i < inbuf-1; ++i) {
    if ( (*(buf+i) == '\r') && (*(buf+i+1) == '\n') ) {
      return i;
    }
  }
  return -1; // return the location of '\r' if found
}

int main(int argc, char* argv[]) {

  if (argc < 2) {
    printf ("To run the program ./mismatch_server <name of input file>\n");
    return 1;
  }
  // read interests from file
  interests = get_list_from_file(argv[1]);
  if (interests == NULL) {
    return 1;
  }
  root = add_next_level(root, interests);

  int fd;
  int nbytes;
  int where; // location of network newline

  struct sockaddr_in peer;
  socklen_t socklen;

  listenfd = setup();
  connected_clients = NULL;

  while (1) {
    socklen = sizeof(peer);

    if ( read_multiplexer(&fd) ) {
      // Ready for accept.
      if ((fd = accept(listenfd, (struct sockaddr *)&peer, &socklen)) < 0) {
        perror("accept");
      } else {
        echo(fd, "What is your user name?\r\n");
        // store temps...
        char temp_ip[17];
        strcpy(temp_ip, inet_ntoa(peer.sin_addr));
        int temp_port = ntohs(peer.sin_port);
        
        connected_clients = add_connected_client(connected_clients, fd, temp_ip, temp_port);
        printf("Connection from %s:%d \n", temp_ip, temp_port);
      }

    } else {
      // Some client is ready for reading
      int remove_flag = 0;
      CNode* curr_user = find_connected_client(connected_clients, fd);
      
      if ( (nbytes = read(fd, curr_user->after, curr_user->room)) > 0) {

        curr_user -> inbuf += nbytes;
        where = find_network_newline(curr_user->buf, curr_user->inbuf);
        if (curr_user -> inbuf >= 1160 || where >= 0) { 
          // OK. we have a full line

          if ( curr_user -> status == AWAITING_USERNAME ) {
            printf("Adding client %s:%d \n", curr_user -> ip, curr_user -> port);
          }
          if ( curr_user -> inbuf >= 1160) {
            curr_user->buf[1258] = '\n';
            curr_user->buf[1159] = '\0';
          } else {
            curr_user->buf[where] = '\n';
            curr_user->buf[where+1] = '\0';
          }
          int execute_result;
          execute_result = execute_command(connected_clients, fd, curr_user->buf);
          if (execute_result == -1) {
            // Remove client
            remove_flag = 1;
          } else {
            if (curr_user -> inbuf >= 1160) {
              // Turncate everything remaining...
              curr_user -> inbuf = 0;
            } else {
              // Shift things left...
              curr_user -> inbuf -= strlen(curr_user->buf)+1;
              // DON'T EAT TOO MUCH HERE!!!
              memmove( curr_user->buf, curr_user->buf+strlen(curr_user->buf)+1, 1184*sizeof(char)-strlen(curr_user->buf) );
            }
          }
        }
        if (remove_flag) {
          printf("Removing client %s \n", curr_user->username);
          connected_clients = remove_connected_client(connected_clients, fd);
          if ( close(fd) ){
            perror("close");
          }
        } else {
          // Update room and after, in preparation for the next read
          curr_user -> after = curr_user->buf + curr_user -> inbuf;
          curr_user -> room = 1184*sizeof(char) - curr_user -> inbuf;
        }
      } else {
        // We are reading an EOF.
        if ( curr_user -> status == AWAITING_USERNAME ) {
          printf("Abnormal disconnection from %s:%d \n", curr_user->ip, curr_user->port);
        } else {
          printf("Removing client %s \n", curr_user->username);
        }
        connected_clients = remove_connected_client(connected_clients, fd);
        if ( close(fd) ){
          perror("close");
        }
      }
      
    }

    
  }
  return 0;
}
   
