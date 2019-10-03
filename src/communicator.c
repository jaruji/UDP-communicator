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

void closeSocket(SOCKET s){
    closesocket(s);
    WSACleanup();
}

void server(int port){                                                                                //server side code, listening on specified port number
    initializeWinsock();
    SOCKET s = createSocket(s);
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    struct sockaddr_in client;
    closeSocket(s);
}

void client(char *ip, int port){
    initializeWinsock();
    SOCKET s = createSocket(s);
    printf("Sending message to port %d, ip %s", port, ip);
    closeSocket(s);
}

void copyFile(char *path, char *buffer, size_t n){
    FILE *f;
    f = fopen(path,"wb");
    fwrite(buffer, 1, n, f);
    fclose(f);
}

char *openFile(char *path) {        //funguje to pog
    size_t n;
    FILE *f = fopen(path, "rb");
    char buffer[250000];
    n = fread(buffer, 1, sizeof(buffer), f);
    fclose(f);
    copyFile("img/test.png", buffer, n);
}

int main(){
    char mode;
    openFile("img/Untitled.png");
    /*
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
            exit(EXIT_SUCCESS);
            break;
        default:
            printf("Wrong input, exiting...\n");
            break;
    }
    return 0;*/
}
