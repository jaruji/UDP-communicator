#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define iplen 15

void server(int port){                                                                                //server side code, listening on specified port number
    char test;
    printf("Server listening on port: %d...\n", port);
    while(1){                                                                                         //waiting for connection
        scanf("%c", &test);
        if(test == 'e'){
            break;
        }
    }
}

void client(char *ip, int port){

}

int main(){
    char mode;
    printf("Select mode by typing one of the following letters: SEND(s) | SEND FILE(f) | RECIEVE(r) | EXIT(e)\n");      //file samostatne?
    char *ip = malloc(iplen * sizeof(char));                                                        //ip address
    int port;                                                                                       //port
    scanf("%c", &mode);
    switch(mode){                                                                                   //switch menu
        case 's':
            printf("Sending mode\n");
            printf("Choose target IP address: ");
            memset(ip, 0, iplen);
            scanf("%s", ip);
            printf("Target IP address is: %s", ip);
            break;
        case 'r':
            printf("Receiving mode\n");
            printf("Choose port to listen on: ");
            scanf("%d", &port);
            server(port);
            break;
        case 'e':
            printf("Exiting...");
            break;
        default:
            printf("Wrong input, exiting...\n");
            break;
    }
    return 0;
}
