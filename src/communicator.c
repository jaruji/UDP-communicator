#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define iplen 15

void initializeWinsock(){
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)                                  //initializing winsock
    {
        exit(EXIT_FAILURE);
    }
    printf("Winsock initialized\n");
}

SOCKET createSocket(SOCKET s){
    if((s = socket(AF_INET , SOCK_DGRAM , 0 )) == INVALID_SOCKET){
        printf("FAILED TO CREATE SOCKET");
        exit(EXIT_FAILURE);
    }
    printf("Socket succesfully created");
    return s;
}

void server(int port){                                                                                //server side code, listening on specified port number
    initializeWinsock();
    SOCKET s = createSocket(s);
}

void client(char *ip, int port){
    initializeWinsock();
    SOCKET s = createSocket(s);
    printf("Sending message to port %d, ip %s", port, ip);
}

int main(){
    char mode;
    printf("Select mode by typing one of the following letters: SEND(s) | RECIEVE(r) | EXIT(e)\n");      //file samostatne?
    char *ip = malloc(iplen * sizeof(char));                                                        //ip address
    int port;                                                                                             //port
    scanf("%c", &mode);
    switch(mode){                                                                                         //switch menu
        case 's':
            printf("Sending mode\n");
            printf("Choose target IP address: ");
            memset(ip, 0, iplen);
            scanf("%s", ip);
            printf("Target IP address is: %s\n", ip);
            printf("Choose target port: ");
            scanf("%d", &port);
            client(ip, port);
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
