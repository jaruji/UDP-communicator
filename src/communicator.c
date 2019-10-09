#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <winsock2.h>

#define iplen 15
#define HEAD 15
#define BUFFLEN 2000000 //for 2MB file
#define DEFAULTSIZE 10
#define MAXFRAGMENT 65527 - HEAD

/*
typedef struct header{
    char buffer[BUFFLEN];            //payload
    char flag;                       //custom flag - S to syn, A to ack, E to end, K to keep alive
    short window;                    //size of fragment that is being sent(in bytes) - without header size
    short checksum;                  //checksum to check for transfer errors
    int number;                      //number of packet
    int count;                       //number of fragments
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


typedef struct acceptedFragment{
    char payload[BUFFLEN];
    int number;
}ACCEPTED;


void help(){
    printf("Made by Juraj Bedej (C)\n");
    printf("Assignment for PKS\n");
    printf("Type :exit to exit the entire program\n");
    printf("Type :file to enter file send mode\n");
    printf("Type :size to choose fragment size (default is 10B)\n");
    printf("Type :error to simulate error in communication\n");
    printf("Type :clear to tidy your screen\n");
    printf("Type :quit to initiate the end of communication\n");
}


void prompt(char *msg){

}


void initializeWinsock(){
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)                                  //initializing winsock
    {
        printf("Error: 'Failed to initialize winsock'");
        exit(EXIT_FAILURE);
    }
    //printf("Winsock initialized\n");
}


SOCKET createSocket(SOCKET s){                                                                      //initalize new socket
    if((s = socket(AF_INET , SOCK_DGRAM , 0 )) == INVALID_SOCKET){
        printf("Error: 'Failed to create socket'");
        exit(EXIT_FAILURE);
    }
    //printf("Socket succesfully created\n");
    return s;
}


void closeSocket(SOCKET s){                                                                          //close socket sent as parameter and cleanup winsock
    closesocket(s);
    WSACleanup();
}


void timeoutServer(SOCKET socket){                                                                    //function that times out the socket
    int timer;
    fd_set fdset;
    struct timeval time;
    FD_ZERO(&fdset);
    FD_SET(socket, &fdset);
    time.tv_sec = 60;
    time.tv_usec = 0;
    timer = select(socket, &fdset, NULL, NULL, &time);
    if(timer == 0){
        //connection times out when it doesnt recieve keepalive packet....
        printf("Connection timed out\n");
        exit(EXIT_SUCCESS);
    }
}


void timeoutClient(SOCKET socket, int try){
    int timer;
    fd_set fdset;
    struct timeval time;
    FD_ZERO(&fdset);
    FD_SET(socket, &fdset);
    time.tv_sec = 20;
    time.tv_usec = 0;
    timer = select(socket, &fdset, NULL, NULL, &time);
    if(timer == 0){
        if(try == 3){
            printf("Error: 'Connection failed'\n");
            exit(EXIT_FAILURE);
        }
        printf("\nNo server response, resending fragments...\n");
        //TODO - implement resending of fragments here!
        return timeoutClient(socket, try += 1);
    }
}

/*
void append(int index, u_char* array, int value, int size){
    int i;
    index += size - 1;
    for(i = 0; i < size; i++){
        array[index-i] = value % 256;
        value /= 256;
    }
}


char *addHeader(FRAGMENT fragment, int number, int count, short checksum, char flag, int window){
    int i, j = 0;
    int len = strlen(fragment.payload + HEAD);
    char *msg = malloc(len * sizeof(char));
    append(0, msg, number, sizeof(number));
    append(4, msg, count, sizeof(count));
    append(8, msg, checksum, sizeof(checksum));
    append(10, msg, window, sizeof(window));
    append(14, msg, flag, sizeof(flag));
    for(i = HEAD; i < len; i++){
        msg[i] = fragment.payload[j++];
    }
    printf("TU JE AJ S HLAVICKOU: %s %d\n", msg, strlen(msg));
    return msg;
    //TODO - takes a fragment and add header, return fragment with added header
    //TODO - files could have different header then messages, but not sure yet
}
*/

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
        //addHeader(str[i], i + 1, fragmentCount, 10, 'X', fragmentSize);
    }
    return str;
}


int count(int len){                                                                                  //count how many fragments will the message be split into
    double x = (double)len / (double)fragmentSize;
    printf("Message will be split into %d fragments", (int)ceil(x));
    return (int)ceil(x);
}


void copyFile(char *path, char *buffer, size_t n){      //copy the contents of buffer to newly created binary file
    FILE *f;
    f = fopen(path,"wb");
    fwrite(buffer, 1, n, f);
    fclose(f);
}


char *openFile(char *path) {                            //open binary file and load its content into a buffer(max 2MB)
    //TODO - bug, file not transfering correctly, worked in previous versions of program
    //TODO - size n is correct, but buffer is only 8b? what
    size_t n;
    FILE *f = fopen(path, "rb");
    if(f == NULL){
        printf("Error: 'file not found'");
        exit(EXIT_FAILURE);
    }
    char buffer[BUFFLEN];
    char *p = buffer;
    n = fread(buffer, 1, sizeof(buffer), f);
    printf("\nSize of file is %d b\n", n);
    fclose(f);
    copyFile("test.png", buffer, n);
    return p;
}


void handshakeClient(SOCKET s, struct sockaddr_in server){
    //TODO - client sends SYN, waits for ACK
    char msg[HEAD];
    strcpy(msg, "SYN");
    msg[strlen(msg)] = '\0';
    //this message is for testing ^
    if(sendto(s, msg, strlen(msg), 0, (struct sockaddr *) &server, sizeof(server)) == SOCKET_ERROR) {
        printf("Error: 'socket error'");
        exit(EXIT_FAILURE);
    }
    memset(msg, 0, strlen(msg));
    timeoutClient(s, 1);
    if(recv(s, msg, HEAD,0) == SOCKET_ERROR){
        printf("Error: 'Socket error'");
        exit(EXIT_FAILURE);
    }
    system("cls");
    printf("Connection established\n");
}


void handshakeServer(SOCKET s, struct sockaddr_in server, struct sockaddr_in client){
    //TODO - server waits for SYN, sends ACK
    char msg[HEAD];
    int len = sizeof(client);
    timeoutServer(s);
    if(recvfrom(s, msg, HEAD,0, (struct sockaddr *) &client, &len) == SOCKET_ERROR){
        printf("Error: 'Socket error'");
        exit(EXIT_FAILURE);
    }
    printf("%s\n", msg);
    memset(msg, 0, strlen(msg));
    strcpy(msg,"ACK");
    if(sendto(s, msg, strlen(msg), 0, (struct sockaddr *) &client, sizeof(client)) == SOCKET_ERROR) {
        printf("Error: 'socket error'");
        exit(EXIT_FAILURE);
    }
    system("cls");
    printf("Connection established\n");
}


void handleFlag(char Flag){
    //TODO - chooses course of action depending on the flag of recieved packet
}


void keepAlive(){
    //TODO - sends keepalive packet if timeout on client side equals 0 - probably multithreading?
}


void checksum(){
    //TODO - calculate the checksum value of packet sent as argument - CRC
}


int checkPacket(){
    //TODO - check checksum value, return 0 if faulty, else 1
}


void server(int port){                                                                                //server side code, listening on specified port number until connection is established
    initializeWinsock();
    SOCKET s = createSocket(s);
    char *msg = malloc(BUFFLEN * sizeof(char));

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;


    struct sockaddr_in client;
    client.sin_family = AF_INET;
    int len = sizeof(client);

    if(bind(s, (struct sockaddr *) &server, sizeof(server)) == SOCKET_ERROR) {
        printf("Error: 'Socket error'");
        exit(EXIT_FAILURE);
    }
    handshakeServer(s, server, client);
    while(1){
        memset(msg, 0, strlen(msg));
        fflush(stdin);
        timeoutServer(s);
        if(recvfrom(s, msg, BUFFLEN,0, (struct sockaddr *) &client, &len) == SOCKET_ERROR){
            printf("Error: 'Socket error'");
            exit(EXIT_FAILURE);
        }

        printf("%s\n", msg);

        if(sendto(s, msg, strlen(msg), 0, (struct sockaddr *) &client, sizeof(client)) == SOCKET_ERROR) {
            printf("Error: 'socket error'");
            exit(EXIT_FAILURE);
        }
    }
    free(msg);
    closeSocket(s);
}


void client(char *ip, int port){
    initializeWinsock();
    SOCKET s = createSocket(s);
    char *msg = malloc(BUFFLEN * sizeof(char));
    char *filename = malloc(100 * sizeof(char));

    struct sockaddr_in server;                                                                   //server sockaddr(for client connection to specified host)
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    handshakeClient(s, server);

    printf("Type ':exit' to exit the interactive console\n");
    printf("Default fragment size is 10b\n");
    printf("Type your message:");
    while(1){                                                                                    //send messages to host in a cycle
        //I need to establish connection first! Add it later(Handshake + keep alive if inactive, connection is ended by client? by :exit probably)
        //connection begins after sending first message/file
        printf("\n>>>");
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
            if(fragmentSize > MAXFRAGMENT){
                fragmentSize = MAXFRAGMENT;
            }
            printf("Fragment size is %d b", fragmentSize);
            continue;
        }
        else if(strcmp(msg, ":file") == 0){
            //TODO- file sending implementation
            memset(msg, 0, strlen(msg));
            printf("Type path to file: ");
            fflush(stdin);
            gets(filename);
            printf("Sending file : %s\n", filename);
            msg = openFile(filename);
        }
        else if(strcmp(msg, ":error") == 0){
            printf("Error msg will be simulated here\n");
        }
        else if(strcmp(msg, ":clear") == 0){
            system("cls");
            continue;
        }
        printf("Message: '%s'\n", msg);
        //##################################
        fragmentCount = count(strlen(msg));
        fragment(msg);
        //##################################
        if(sendto(s, msg, strlen(msg), 0, (struct sockaddr *) &server, sizeof(server)) == SOCKET_ERROR) {
            printf("Error: 'socket error'");
            exit(EXIT_FAILURE);
        }
        memset(msg, 0, strlen(msg));
        timeoutClient(s, 1); //it worked before, doesnt work anymore WTF? actually cursed programming
        if(recv(s, msg, BUFFLEN,0) == SOCKET_ERROR){
            printf("Error: 'Socket error'");
            exit(EXIT_FAILURE);
        }

        printf("\nServer response: %s", msg);
        memset(msg, 0, strlen(msg));
        memset(filename, 0, strlen(filename));
    }
    free(msg);
    closeSocket(s);
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
