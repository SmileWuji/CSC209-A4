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


void echo(int fd, char* content) {
    if ( write(fd, content, strlen(content)*sizeof(char)) == -1 ) {
        perror("write");
    }
}


void feed_question(int fd, CNode *user) {
    echo(fd, "Do you like ");
    echo(fd, user -> next_question -> question);
    echo(fd, "?\r\n");
}


// print list of potential mismatches for user (on the client interface)
void write_mismatches(int fd, Node *list, char *name){
    int friends = 0;

    // iterate over user list and count the number of friends
    while (list) {
    // ignore this user
        if (strcmp(list->str, name)) {
            friends++;
             
        // if this is the first friend found, print successful message    
            if (friends == 1)
                echo(fd, "Here are your best mismatches:");
            
        // if friend was found, print his/her name
            echo(fd, "\n");
            echo(fd, list->str);
        }
            
        list = list->next;
    }
    
    // if friends were found, print the number of friends    
    if (friends){
        echo(fd, "\r\n");
        
    } else {
        echo(fd, "No completing personalities found. Please try again later.\r\n" );
    }
}


/* Returns -1 for quit,
   Returns 0 for no,
   Returns 1 for yes,
   Returns 2 for invalid input.
*/
int validate_answer(char *answer){
    if (strlen(answer) > 3){
        return 2;
    }
    
    if (answer[0] == 'q' || answer[0] == 'Q')
        return -1;
    if (answer[0] == 'n' || answer[0] == 'N')
        return 0;
        
    if (answer[0] == 'y' || answer[0] == 'Y')
        return 1;
    return 2;
}


/* Execute the command according to this given fd.
 * DO NOT CHANGE THE ORIGINAL buf!
 * Also, notice that there's a newline character '\n' at the end of buf.
 *
 * Returns -1 when "quit".
 * Returns 0 otherwise.
 */
int execute_command(CNode* head, int fd, char* buf) {
    // Copy what's inside buf and turncate the string.
    int buf_length = strlen(buf);
    char input[buf_length];
    strncpy(input, buf, buf_length);
    input[buf_length-1] = '\0';
    char input_copy[buf_length];
    strcpy(input_copy, input);

    // Get the user.
    CNode* user = find_connected_client(head, fd);
    
    if ( user->status == AWAITING_USERNAME ) {
        if ( strlen(input)>128 ) {
            echo(fd, "Username being turncated into 128 characters.\r\n");
            input[127] = '\0';
        }
        if ( find_client_by_username(head, input) != NULL ) {
            echo(fd, "A user with a same name has already connected.\nYou are forced to disconnect.\r\n");
            return -1;
        }
        strcpy(user -> username, input);
        echo(fd, "Welcome.\nGo ahead and enter user commands>\r\n");
        
        Node * belonging_list = find_user(root, input);
        if ( belonging_list == NULL ) {
            user -> status = NEW_USER;
            user -> next_question = root;
            // indicate the "next_question"
            user -> cursor = interests;
        } else {
            user -> status = IDLE;
            user -> target_list = find_completely_opposite();
        }
        return 0;
    } else if ( strcmp(input, "quit") == 0 ) {
        return -1;

    } else if ( user->status == DOING_TEST ) {
        int ans_code = validate_answer(input);
        if (ans_code == -1) {
            return -1;
        } else if (ans_code == 2) {
            echo(fd, "ERROR: Answer must be one of 'y', 'n', 'q'.\r\n");
            return 0;
        } else {
            QNode* nxt = find_branch(user->next_question, ans_code);
            user -> cursor = user -> cursor -> next;
            // Test done!
            if ( user -> cursor == NULL ) {
                Node* list = user -> next_question -> children[ans_code].fchild;
                char* name = user -> username;
                user -> next_question -> children[ans_code].fchild = add_user(list, name);
                find_user(root, name);
                user -> target_list = find_completely_opposite();
                user -> status = IDLE;
                printf("Recorded activity for %s\n", user -> username);
                echo(fd, "Test complete.\r\n");

            } else {
                user -> next_question = nxt;
                feed_question(fd, user);
            }
            return 0;
        }

    }

    // Split the command and other input as message...
    // NOTE: We should check whether target_username == NULL before
    //      we use message.
    // NOTE: message contains a newline character at the end.
    int input_length = strlen(input);
    if ( input_length == 0 ) {
        /* Empty string.*/
        echo(fd, "Go ahead and enter user commands>\r\n");
        return 0;
    }
    char* command = strtok(input, DELIM);
    char* target_username = strtok(NULL, DELIM);
    char* message = NULL;

    if ( target_username != NULL ) {
        char* third_token = strtok(NULL, DELIM);
        if ( third_token != NULL ) {
            message = strstr( input_copy+strlen(command)+strlen(target_username)+2, third_token );
        }
    }

    if ( strcmp(command, "do_test") == 0 && target_username == NULL) {
        /* The specified user is ready to start answering questions. You
         * need to make sure that the user answers each question only
         * once.
         */
        if (user -> status != NEW_USER) {
            echo(fd, "You've done the test already.\r\n");
            return 0;
        }
        user -> status = DOING_TEST;
        feed_question(fd, user);

    } else if ( strcmp(command, "get_all") == 0 && target_username == NULL) {
        /* Send the list of best mismatches related to the specified
         * user. If the user has not taked the test yet, return the
         * corresponding error value (different than 0 and -1).
         */
        if (user -> status != IDLE) {
            echo(fd, "You need to do the test first.\r\n");
            return 0;
        }
        /* No user to new user means from null to non-null.
           We might need to assign the list again.
        */
        if (user -> target_list == NULL) {
            find_user(root, user -> username);
            user -> target_list = find_completely_opposite();
        }
        write_mismatches(fd, user -> target_list, user -> username);

    } else if (strcmp(command, "post") == 0 ) {
        /* Send the specified message stored in cmd_argv[2] to the user
         * stored in cmd_argv[1].
         */
        if ( target_username == NULL || message == NULL ) {
            echo(fd, "Incorrect syntax.\nUsage: post <target_username> <stuff...>\r\n");
            return 0;
        }

        CNode* buddy = find_client_by_username(head, target_username);
        if (buddy == NULL) {
            echo(fd, "ERROR: User ");
            echo(fd, target_username);
            echo(fd, " is offline or hasn't registered yet. \nPlease try again later.\r\n");     
        } else {
            echo(buddy->fd, "Message from ");
            echo(buddy->fd, user->username);
            echo(buddy->fd, ": ");
            echo(buddy->fd, message);
            echo(buddy->fd, "\r\n");
        }

    } else {
        /* The input message is not properly formatted. */
        echo(fd, "command not supported\r\n");
    }

    return 0;
}


/* PRECONDITION: fd is not inside the list of connected clients.
   Returns the new head.
*/
CNode* add_connected_client(CNode* head, int fd, char* ip, int port){
    CNode* node = calloc(1, sizeof(CNode));
    node -> fd = fd;
    strcpy(node -> ip, ip);
    node -> port = port;
    node -> next = head;
    node -> status = AWAITING_USERNAME;
    node -> inbuf = 0;
    node -> room = 1184*sizeof(char);
    node -> after = node->buf;
    return node;
}


/* Returns NULL if not found.
   Returns the node.
*/
CNode* find_connected_client(CNode* head, int fd){
    while (head != NULL) {
        if ((head -> fd) == fd) {
            return head;
        } else {
            head = head -> next;
        }
    }
    return NULL;
}

/* Returns NULL if not found.
   Returns the node.
*/
CNode* find_client_by_username(CNode* head, char* target_name){
    while (head != NULL) {
        if (strcmp(head -> username, target_name) == 0) {
            return head;
        } else {
            head = head -> next;
        }
    }
    return NULL;
}


/* PRECONDITION: fd is not inside the list of connected clients.
   Returns the new head.
*/
CNode* remove_connected_client(CNode* head, int fd){
    if (head->fd == fd) {
        CNode* new_head = NULL;
        new_head = head -> next;
        free(head);
        return new_head;
    } 
    CNode* curr = head;
    CNode* next = head -> next;
    while (next -> fd != fd) {
        curr = next;
        next = next -> next;
    }
    curr -> next = next -> next;
    free(next);

    return head;
}


