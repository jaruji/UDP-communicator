#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <winsock2.h>

#define iplen 15
#define HEAD ?
#define BUFFLEN 2000000 //for 2MB file
#define DEFAULTSIZE 10

/*
typedef struct header{
    char buffer[BUFFLEN];            //payload
    char flag;                       //custom flag - S to syn, A to ack, E to end, K to keep alive
    int size;                        //size of fragment that is being sent(in bytes) - without header size
    int checksum;                    //checksum to check for transfer errors
    int number;                      //number of packet
}HEADER;
*/

//######################################################################################
//global variables here
int fragmentCount;                  //number of fragments
int fragmentSize = DEFAULTSIZE;     //maximum size of one fragment
//######################################################################################


typedef struct fragment{
    char payload[BUFFLEN];
}FRAGMENT;


void help(){
    printf("Made by Juraj Bedej (C)\n");
    printf("Assignment for PKS\n");
    printf("Type :exit to exit the interactive console\n");
    printf("Type :file <filepath> to send a file\n");
    printf("Type :size to choose fragment size (default is 10b)\n");
    //printf("");
}


void initializeWinsock(){
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)                                  //initializing winsock
    {
        printf("Error: 'Failed to initialize winsock'");
        exit(EXIT_FAILURE);
    }
    printf("Winsock initialized\n");
}


SOCKET createSocket(SOCKET s){                                                                      //initalize new socket
    if((s = socket(AF_INET , SOCK_DGRAM , 0 )) == INVALID_SOCKET){
        printf("Error: 'Failed to create socket'");
        exit(EXIT_FAILURE);
    }
    printf("Socket succesfully created\n");
    return s;
}


void closeSocket(SOCKET s){                                                                          //close socket sent as parameter and cleanup winsock
    closesocket(s);
    WSACleanup();
}

void timeout(SOCKET socket){
    int timer;
    fd_set fdset;
    struct timeval time;
    FD_ZERO(&fdset);
    FD_SET(socket, &fdset);
    time.tv_sec = 60;
    time.tv_usec = 0;
    timer = select(socket, &fdset, NULL, NULL, &time);
    if(timer == 0){
        printf("Connection timed out\n");
        exit(EXIT_SUCCESS);
    }
}


FRAGMENT *fragment(char *msg){                                                                      //fragmenting the message
    int i, j, curr = 0;
    FRAGMENT *str = malloc(fragmentCount* sizeof(FRAGMENT));
    memset(str, 0, sizeof(str));
    for(i = 0; i < fragmentCount; i++){
        for(j = 0; j < fragmentSize; j++){
            str[i].payload[j] = msg[curr];
            curr++;
        }
        str[i].payload[curr] = '\0';
    }
    for(i = 0; i < fragmentCount; i++){
        printf("\nFragment number %d: %s", i + 1, str[i].payload);
    }
    return str;
}


int count(int len){                                                                                  //count how many fragments will the message be split into
    double x = (double)len / (double)fragmentSize;
    printf("Message will be split into %d fragments", (int)ceil(x));
    return (int)ceil(x);
}


void server(int port){                                                                                //server side code, listening on specified port number until connection is established
    initializeWinsock();
    SOCKET s = createSocket(s);
    char *msg = malloc(BUFFLEN * sizeof(char));

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;

    if(bind(s, (struct sockaddr *) &server, sizeof(server)) == SOCKET_ERROR){
        printf("Error: 'Socket error'");
        exit(EXIT_FAILURE);
    }
    printf("Socket binding successful\n");
    while(1){
        memset(msg, 0, strlen(msg));
        fflush(stdin);
        printf("...\n");
        timeout(s);
        if(recv(s, msg, BUFFLEN,0) == SOCKET_ERROR){
            printf("Error: 'Socket error'");
            exit(EXIT_FAILURE);
        }
        printf("%s\n", msg);
    }
    free(msg);
    closeSocket(s);
}


void client(char *ip, int port){
    initializeWinsock();
    SOCKET s = createSocket(s);
    char *msg = malloc(BUFFLEN * sizeof(char));

    struct sockaddr_in server;                                                                   //server sockaddr(for client connection to specified host)
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    printf("Type ':exit' to exit the interactive console\n");
    printf("Default fragment size is 10b\n");
    printf("Type your message:");
    while(1){                                                                                    //send messages to host in a cycle
        //I need to establish connection first! Add it later(Handshake + keep alive if inactive, connection is ended by client? by :exit probably)
        //connection begins after sending first message/file
        printf("\n>>");
        fflush(stdin);                                                                           //flush the standart input
        gets(msg);
        if(strcmp(msg, ":exit") == 0){                                                          //exit the console
            printf("Exiting...");
            break;
        }
        else if(strcmp(msg, ":help") == 0) {                                                    //help menu
            help();
            continue;
        }
        else if(strcmp(msg, ":size") == 0) {                                                     //choose the maximum size of one fragment
            printf("Select fragment size in b:  ");
            scanf("%d", &fragmentSize);
            printf("Fragment size is %d b", fragmentSize);
            continue;
        }
        else if(strcmp(msg, ":file") == 0){
            fflush(stdin);
            printf("Sending a file will be implemented here\n");
            continue;
        }
        else if(strcmp(msg, ":error") == 0){
            printf("Error msg will be simulated here\n");
        }
        printf("Message: '%s'\n", msg);
        //##################################
        fragmentCount = count(strlen(msg));
        fragment(msg);
        if(sendto(s, msg, strlen(msg), 0, (struct sockaddr *) &server, sizeof(server)) == SOCKET_ERROR){
            printf("Error: 'socket error'");
            exit(EXIT_FAILURE);
        }
        memset(msg, 0, strlen(msg));
    }
    free(msg);
    closeSocket(s);
}


void copyFile(char *path, char *buffer, size_t n){      //copy the contents of buffer to newly created binary file
    FILE *f;
    f = fopen(path,"wb");
    fwrite(buffer, 1, n, f);
    fclose(f);
}


char *openFile(char *path) {                            //open binary file and load its content into a buffer(max 2Mb)
    size_t n;
    FILE *f = fopen(path, "rb");
    char buffer[BUFFLEN];
    n = fread(buffer, 1, sizeof(buffer), f);
    fclose(f);
    copyFile("img/test.png", buffer, n);
}


int main(){
    char mode;
    //openFile("img/Untitled.png");

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
            printf("Error: 'Wrong input, exiting...'\n");
            break;
    }
    return 0;
}
