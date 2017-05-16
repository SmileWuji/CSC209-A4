#ifndef _UTILS_H
#define _UTILS_H

#include <netinet/in.h>
#include "qtree.h"

#define MAX_NAME 128
#define INPUT_ARG_MAX_NUM 3
#define DELIM " \r\n"

#define AWAITING_USERNAME 0
#define NEW_USER 1
#define DOING_TEST 2
#define IDLE 3

/*
 * Τhe definition of the following structure is copied directly from the
 * muffinman server (http://www.cdf.toronto.edu/~ajr/209/a4/muffinman.c).
 * You might need to add more fields - such as a seoarate buffer for each client and 
 * the current position in the buffer - as explained in A 4 handout.
 */
typedef struct client {
    int status;     // AWAITING_USERNAME, NEW_USER, DOING_TEST, IDLE
    int fd;         // the current fd... only useful when ONLINE
    char ip[17];
    int port;
    char username[MAX_NAME];
    struct client *next;
    QNode* next_question;
    Node* target_list;
    Node* cursor;
    char buf[1184];
    int inbuf;
    int room;
    char* after;
} CNode;

/* Execute the command according to this given fd.
 * DO NOT CHANGE THE ORIGINAL buf!
 * Also, notice that there's a newline character '\n' at the end of buf.
 */
int execute_command(CNode* head, int fd, char* buf);

void echo(int fd, char* content);

CNode* add_connected_client(CNode* head, int fd, char* ip, int port);
CNode* find_connected_client(CNode* head, int fd);
CNode* find_client_by_username(CNode* head, char* target_name);
CNode* remove_connected_client(CNode* head, int fd);

Node *interests;
QNode *root;

#endif /* _UTILS_H */
