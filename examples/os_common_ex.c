#include <stdio.h>

#define MM_IMPLEMENT
#include "../os_common.h"

int main() {
    
    int rc = os_common_startup();
    if (rc != 0) {
        fprintf(stderr, "Could not start up properly\n");
        return 1;
    }
    
    sockfd sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == INVALID_SOCKET) {
        fprintf(stderr, "Could not open socket: %s\n", sockstrerror(sockerrno));
        return -1;
    }
    
    closesocket(sfd);
    
    os_common_cleanup();
    
    puts("done");
    return 0;
}
