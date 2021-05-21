#include <stdio.h>

#define MM_IMPLEMENT
#include "../os_common.h"

#define BUF_SZ 64

//Helper function to send a buffer over a TCP socket
//This could do error checking but whatever...
int send_buffer(sockfd sfd, char *msg, int msglen) {
    int sent = 0;
    while (sent < msglen) {
        int num = write(sfd, msg+sent, msglen - sent);
        sent += num;
    }
    return 0;
}

int main() {
    
    int rc = os_common_startup();
    if (rc != 0) {
        fprintf(stderr, "Could not start up properly\n");
        return 1;
    }
    
    sockfd sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == INVALID_SOCKET) {
        fprintf(stderr, "Could not open socket: %s\n", sockstrerror(sockerrno));
        return -1; //Lazy... should technically release resources
    }
    
    struct sockaddr_in my_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(4567),
        .sin_addr = {INADDR_ANY}
    };
    
    rc = fix_rc(
        bind(
            sfd, 
            (struct sockaddr *) &my_addr, 
            sizeof(struct sockaddr_in)
        )
    );
    if (rc != 0) {
        fprintf(stderr, "Could not bind socket: %s\n", sockstrerror(rc));
        return -1; //Lazy... should technically release resources
    }
    
    rc = listen(sfd, 1);
    if (rc != 0) {
        fprintf(stderr, "Could not listen for incoming connections: %s\n", sockstrerror(rc));
        return -1; //Lazy... should technically release resources
    }
    
    sockfd client_sfd = accept(sfd, NULL, NULL);
    if (client_sfd == INVALID_SOCKET) {
        fprintf(stderr, "Something went wrong accepting a client connection: %s\n", sockstrerror(sockerrno));
        return -1; //Lazy... should technically release resources
    } else {
        puts("Connection accepted!");
    }
    
    while(1) {
        char buf[BUF_SZ];
        int numRead = read(client_sfd, buf, BUF_SZ);
        if (numRead == 0) {
            puts("Connection closed");
            break;
        }
        send_buffer(client_sfd, buf, numRead);
    }
    
    closesocket(sfd);
    
    os_common_cleanup();
    
    puts("done");
    return 0;
}
