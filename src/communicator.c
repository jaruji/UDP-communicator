#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <winsock2.h>
//#include <pthread.h>

#define iplen 15
#define HEAD 9
#define BUFFLEN 2000000 //for 2MB file
#define DEFAULTSIZE 10
#define MAXFRAGMENT 1472 - HEAD
#define NAMESIZE 100
#define LIMIT 5


typedef struct fragment{
    short len;
    u_char *payload;
}FRAGMENT;


typedef struct acceptedFragment{
    int seq;
    u_char *payload;
    short len;
}ACCEPTED;


//######################################################################################
//global variables here
int fragmentCount;                  //number of fragments
int fragmentSize = DEFAULTSIZE;     //maximum size of one fragment
int polynome = 0xd175;             //generator polynome for CRC-16
//######################################################################################


void help(){
    printf("Made by Juraj Bedej (C)\n");
    printf("Assignment for PKS\n");
    printf("Type :exit to exit end communication\n");
    printf("Type :file to enter file send mode\n");
    printf("Type :size to choose fragment size (default is 10B)\n");
    printf("Type :error to simulate error in communication\n");
    printf("Type :clear to tidy your screen\n");
}


int prompt(char *msg){
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


void timeoutServer(SOCKET socket){                                                                    //function that times out the socket if no keepalive or message received
    int timer;
    fd_set fdset;
    struct timeval time;
    FD_ZERO(&fdset);
    FD_SET(socket, &fdset);
    time.tv_sec = 60;                                                                                  //timeout after 60 seconds
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
        printf("\nNo server response, trying again...\n");
        //TODO - implement resending of fragments here!
        return timeoutClient(socket, try += 1);                         //recursive call of function timeoutClient()
    }
}


void append(int index, u_char *array, int value, int size){
    int i;
    index += size - 1;
    for(i = 0; i < size; i++){
        array[index-i] = value % 256;
        value /= 256;
    }
}


u_char *addHeader(char *fragment, int number, short checksum, char flag, short window){
    int i, j = 0;
    int len = strlen(fragment) + HEAD;
    u_char *msg = malloc(len * sizeof(u_char));
    append(0, msg, number, sizeof(number));
    append(4, msg, checksum, sizeof(checksum));
    append(6, msg, window, sizeof(window));
    append(8, msg, flag, sizeof(flag));
    for(i = HEAD; i < len; i++){
        msg[i] = fragment[j++];
    }
    msg[i] = '\0';
    return msg;
}


void print(int num, u_char *msg, int len){
    int i;
    printf("\n %d. :", num);
    for(i = 0; i < len; i++){
        printf(" %.2x ", msg[i]);
    }
    printf("\n");
}


int myFree(FRAGMENT *p){
    if(p == NULL)
        return 0;
    int i;
    for(i = 0; i < fragmentCount; i++){
        free(p[i].payload);
    }
    free(p);
    return 1;
}


int count(int len){                                                                                  //count how many fragments will the message be split into
    double x = (double)len / (double)fragmentSize;
    int ret = (int)ceil(x);
    printf("Message will be split into %d fragments", ret);
    return ret;
}


short checksum(u_char *fragment, int len){                                                                   //CRC-16 checksum
    //TODO - calculate the checksum value of packet sent as argument - probably CRC
    //unsigned char i, j, crc = 0;
    //**********************************************************************************************************************
    //code taken from https://www.pololu.com/docs/0J44/6.7.6
    int i, j;
    short crc = 0;
    for (i = 0; i < len; i++){
        crc ^= fragment[i];
        for (j = 0; j < 17; j++){
            if (crc & 1)
            crc ^= polynome;
            crc >>= 1;
        }
    }
    return crc;
    //**********************************************************************************************************************
}


FRAGMENT *fragment(char *msg){                                                                      //fragmenting the message
    int i, j, curr = 0;
    char flag = 'M';
    fragmentCount = count(strlen(msg));
    FRAGMENT *str = malloc(fragmentCount * sizeof(FRAGMENT));
    for(i = 0; i < fragmentCount; i++){
        str[i].payload = calloc(fragmentSize + 1, sizeof(u_char));
    }
    for(i = 0; i < fragmentCount; i++){
        for(j = 0; j < fragmentSize; j++){
            str[i].payload[j] = msg[curr++];
        }
        str[i].payload[j] = '\0';
        printf("\nFRAGMENT %d: %s", i+1, str[i].payload);
    }
    u_char *tmp;
    for(i = 0; i < fragmentCount; i++){
        tmp = str[i].payload;
        str[i].len = HEAD + strlen(str[i].payload);
        if(i == fragmentCount - 1)
            flag = 'L';
        str[i].payload = addHeader(str[i].payload, i + 1, checksum(str[i].payload, str[i].len), flag, fragmentSize);
        free(tmp);
        print(i + 1, str[i].payload, str[i].len);
    }
    return str;
}


char *decodeMessage(u_char *pkt_data, int len){                 //extracts the data segment from fragment
    int i, j = 0;
    char *message = malloc((len - HEAD) * sizeof(char));
    for(i = HEAD; i < len; i++){
        message[j++] = pkt_data[i];
    }
    message[j] = '\0';
    return message;
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


int handleFlag(char flag){
    //TODO - chooses course of action depending on the flag of recieved packet
    switch(flag){
        case('S'):
            printf("Recieved SYN\n");
            return 0;
        case('K'):
            printf("\nRecieved KEEPALIVE\n");
            return 0;
        case('A'):
            printf("\nReceived ACK\n");
            return 0;
        case('L'):
            printf("\nLast fragment recevied\n");
            return 1;
        case('E'):
            printf("\nExiting communication\n");
            return 2;
    }
}


void handshakeClient(SOCKET s, struct sockaddr_in server){
    //TODO - client sends SYN, waits for ACK
    //this message is for testing ^
    char msg[HEAD];
    system("cls");
    printf("Sending SYN\n");
    if(sendto(s, addHeader("", 1, 0, 'S', 0), HEAD, 0, (struct sockaddr *) &server, sizeof(server)) == SOCKET_ERROR) {
        printf("Error: 'socket error'");
        exit(EXIT_FAILURE);
    }
    timeoutClient(s, 1);
    if(recv(s, msg, HEAD,0) == SOCKET_ERROR){
        printf("Error: 'Socket error'");
        exit(EXIT_FAILURE);
    }
    print(1, msg, HEAD);
    handleFlag(msg[8]);
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
    system("cls");
    handleFlag(msg[8]);
    print(1, msg, HEAD);
    printf("\nSending ACK\n");
    if(sendto(s, addHeader("",2,0,'A',0), HEAD, 0, (struct sockaddr *) &client, sizeof(client)) == SOCKET_ERROR) {
        printf("Error: 'socket error'");
        exit(EXIT_FAILURE);
    }
    printf("Connection established\n");
}


void end(SOCKET s, struct sockaddr_in server){
    char msg[HEAD];
    if(sendto(s, addHeader("",1,0,'E',0), HEAD, 0, (struct sockaddr *) &server, sizeof(server)) == SOCKET_ERROR) {
        printf("Error: 'socket error'");
        exit(EXIT_FAILURE);
    }
    memset(msg, 0, HEAD);
    timeoutClient(s, 1);
    if(recv(s, msg, HEAD,0) == SOCKET_ERROR){
        printf("Error: 'Socket error'");
        exit(EXIT_FAILURE);
    }
    print(1, msg, HEAD);
}


void *keepAlive(void *vargp){
    while(1) {
        Sleep(10000);
        printf("K den");
    }
}


int checkPacket(u_char *fragment){
    //TODO - check checksum value, return 0 if faulty, else 1

}


void requestFragments(int *missing){
    //TODO - if fragments arrive faulty, server sends a request for them to be resent
}


int toInt(u_char *data, int index){
    return data[index] * pow(256, 3) + data[index + 1] * pow(256,2) + data[index + 2] * 256 + data[index + 3];
}


short toShort(u_char *data, int index){
    return data[index] * 256 + data[index + 1];
}


int comp (const void * a, const void * b)
{
    ACCEPTED *x = (ACCEPTED*) a;
    ACCEPTED *y = (ACCEPTED*) b;
    return x->seq - y->seq;
}

char* orderFragments(int len, ACCEPTED *acceptedFragments){
    int i;
    qsort(acceptedFragments, len, sizeof(ACCEPTED), comp);
}


void server(int port){                                                                                //server side code, listening on specified port number until connection is established
    initializeWinsock();
    SOCKET s = createSocket(s);
    u_char *msg = malloc(MAXFRAGMENT * sizeof(u_char));

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

    handshakeServer(s, server, client);                                                                   //server waits for SYN, responds with ACK

    while(1){
        memset(msg, 0, MAXFRAGMENT);
        //fflush(stdin);
        int recvb = 0, i = 1, state = 1;
        while(state) {
            timeoutServer(s);
            if(recvfrom(s, msg, MAXFRAGMENT, 0, (struct sockaddr *) &client, &len) == SOCKET_ERROR) {
                printf("Error: 'Socket error'");
                exit(EXIT_FAILURE);
            }
            recvb = toShort(msg, 6) + HEAD;                     //size of received fragment equals window size + header size
            print(i, msg, recvb);
            switch(handleFlag(msg[8])){
                case 2:
                    print(1, msg, HEAD);
                    if (sendto(s, addHeader("", 2, 0, 'A', 0), HEAD, 0, (struct sockaddr *) &client, sizeof(client)) == SOCKET_ERROR) {
                        printf("Error: 'socket error'");
                        exit(EXIT_FAILURE);
                    }
                    free(msg);
                    closeSocket(s);
                    return;
                case 1:
                    state = 0;
                    break;
            }
            memset(msg, 0, recvb);
            if (sendto(s, addHeader("", i, 0, 'A', 0), HEAD, 0, (struct sockaddr *) &client, sizeof(client)) == SOCKET_ERROR) {
                printf("Error: 'socket error'");
                exit(EXIT_FAILURE);
            }
            i++;
        }
    }
}


void client(char *ip, int port){
    initializeWinsock();
    SOCKET s = createSocket(s);
    //pthread_t tID;                                                                              //Posix thread for keepalive prob.
    FRAGMENT *p = NULL;
    char *msg = malloc(BUFFLEN * sizeof(char));
    char *filename = malloc(NAMESIZE * sizeof(char));

    struct sockaddr_in server;                                                                   //server sockaddr(for client connection to specified host)
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    handshakeClient(s, server);                                                                  //client sends SYN flag, waits for ACK

    printf("Type ':exit' to exit the interactive console\n");
    printf("Default fragment size is 10B\n");
    printf("Type your message:");
    //pthread_create(&tID, NULL, keepAlive, NULL);
    while(1){                                                                                    //send messages to host in a cycle
        if(myFree(p)){
            p = NULL;
        }
        printf("\n>>>");
        fflush(stdin);                                                                           //flush the standart input
        gets(msg);
        if(strcmp(msg, ":exit") == 0){                                                          //exit the console
            printf("Exiting communication\n");
            end(s,server);
            break;
        }
        else if(strcmp(msg, ":help") == 0) {                                                    //help menu
            help();
            continue;
        }
        else if(strcmp(msg, ":size") == 0) {                                                     //choose the maximum size of one fragment
            printf("Select fragment size in B:  ");
            scanf("%d", &fragmentSize);
            if(fragmentSize > MAXFRAGMENT){
                fragmentSize = MAXFRAGMENT;
            }
            else if(fragmentSize < 1)
                fragmentSize = 1;
            printf("Fragment size is %d B", fragmentSize);
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
        else if(strcmp(msg, ":clear") == 0) {
            system("cls");
            continue;
        }
        printf("Your message: '%s'\n", msg);
        //##################################
        p = fragment(msg);
        //##################################
        int i = 0, recvb;
        while(i < fragmentCount) {
            if (sendto(s, p[i].payload, p[i].len, 0, (struct sockaddr *) &server, sizeof(server)) == SOCKET_ERROR) {
                printf("Error: 'socket error'");
                exit(EXIT_FAILURE);
            }
            memset(msg, 0, strlen(msg));
            timeoutClient(s, 1);
            if (recv(s, msg, BUFFLEN, 0) == SOCKET_ERROR) {
                printf("Error: 'Socket error'");
                exit(EXIT_FAILURE);
            }
            recvb = toShort(msg, 6) + HEAD;
            printf("\nServer response: ");
            print(i + 1, msg, recvb);
            memset(msg, 0, recvb);
            memset(filename, 0, strlen(filename));
            i++;
        }
    }
    free(msg);
    free(filename);
    closeSocket(s);
}

int main(){
    char mode;
    char *ip = malloc(iplen * sizeof(char));                                                        //ip address
    int port;
    //openFile("img/Untitled.png");
    while(1) {
        memset(ip, 0, iplen);
        printf("Select mode by typing one of the following letters: SEND(s) | RECIEVE(r) | EXIT(e)\n");      //file samostatne?
        fflush(stdin);
        scanf("%c", &mode);
        switch (mode) {                                                                                         //switch menu
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
            default:
                printf("Error: 'Wrong input, try again...'\n");
                break;
        }
    }
}
