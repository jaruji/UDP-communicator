#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <winsock2.h>
#include <pthread.h>

#define iplen 15
#define HEAD 9
#define BUFFLEN 2000000 //for 2MB file
#define DEFAULTSIZE 10
#define MAXFRAGMENT 1500 - 20 - 8 - HEAD
#define LIMIT 5

//######################################################################################
//global variables here
int fragmentCount;                  //number of fragments
int fragmentSize = DEFAULTSIZE;     //maximum size of one fragment
int generator = 0xd175;             //generator polynome for CRC-16
//######################################################################################


typedef struct fragment{
    int len;
    u_char *payload;
}FRAGMENT;


typedef struct acceptedFragment{
    u_char *payload;
    int number;
}ACCEPTED;


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
        printf("\nNo server response, trying again...\n");
        //TODO - implement resending of fragments here!
        return timeoutClient(socket, try += 1);                         //recursive call of function timeoutClient()
    }
}


void append(int index, u_char* array, int value, int size){
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
    //memcpy(msg, &number, sizeof(number));
    //memcpy(&msg[sizeof(number)], &checksum, sizeof(checksum));
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


FRAGMENT *fragment(char *msg){                                                                      //fragmenting the message
    int i, j, curr = 0;
    FRAGMENT *str = malloc(fragmentCount* sizeof(FRAGMENT));
    for(i = 0; i < fragmentCount; i++){
        str[i].payload = malloc(fragmentSize * sizeof(FRAGMENT));
    }
    memset(str, 0, sizeof(str));
    for(i = 0; i < fragmentCount; i++){
        //str[i].payload = malloc(fragmentSize * sizeof(u_char));
        for(j = 0; j < fragmentSize; j++){
            str[i].payload[j] = msg[curr];
            curr++;
        }
        str[i].payload[curr] = '\0';
        str[i].len = HEAD + strlen(str[i].payload);
    }
    for(i = 0; i < fragmentCount; i++){
        //printf("\nFragment number %d: %s", i + 1, str[i].payload);
        print(i + 1,addHeader(str[i].payload, i + 1, 1000, 'M', fragmentSize), str[i].len);
        //print(i + 1,str[i].payload);
    }
    return str;
}


void myFree(FRAGMENT *str){
    int i;
    for(i = 0; i < fragmentCount; i++){
        free(str[i].payload);
    }
    free(str);
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
            printf("\nRecieved ACK\n");
            return 0;
        case('L'):
            printf("\nLast fragment recieved\n");
            return 1;
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
    //memset(msg, 0, strlen(msg));
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
    memset(msg, 0, strlen(msg));
    if(sendto(s, addHeader("",2,0,'A',0), HEAD, 0, (struct sockaddr *) &client, sizeof(client)) == SOCKET_ERROR) {
        printf("Error: 'socket error'");
        exit(EXIT_FAILURE);
    }
    printf("Connection established\n");
}


void *keepAlive(void *vargp){
    while(1) {
        Sleep(10000);
        printf("K");
    }
}


short checksum(u_char *fragment){
    //TODO - calculate the checksum value of packet sent as argument - probably CRC

}


int checkPacket(u_char *fragment){
    //TODO - check checksum value, return 0 if faulty, else 1

}


void requestFragments(int *missing){
    //TODO - if fragments arrive faulty, server sends a request for them to be resent
}


void server(int port){                                                                                //server side code, listening on specified port number until connection is established
    initializeWinsock();
    SOCKET s = createSocket(s);
    u_char *msg = malloc(BUFFLEN * sizeof(u_char));

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
        int i = 0, j = 1, k = 1, recvlen;
        memset(msg, 0, strlen(msg));
        fflush(stdin);
        timeoutServer(s);
        for(i = 0; i < LIMIT; i++) {
            if (recvlen = recvfrom(s, msg, BUFFLEN, 0, (struct sockaddr *) &client, &len) == SOCKET_ERROR) {
                printf("Error: 'Socket error'");
                exit(EXIT_FAILURE);
            }
            printf("Recieved %d bytes\n", recvlen);
            print(j++, msg, recvlen);
            if(handleFlag(msg[8])){
                break;
            }
        }
        //printf("%s\n", msg);

        if(sendto(s, addHeader("",k++,0,'A',0), HEAD, 0, (struct sockaddr *) &client, sizeof(client)) == SOCKET_ERROR) {
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
    pthread_t tID;
    FRAGMENT *fp;
    char *msg = malloc(BUFFLEN * sizeof(char));
    char *filename = malloc(100 * sizeof(char));

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
        printf("Message: '%s'\n", msg);
        //#######################################################################################################################################################################
        fragmentCount = count(strlen(msg));
        fp = fragment(msg);
        //addHeader(fp[i].payload, i + 1, 10000, 'M', fragmentSize)
        int i = 0, j = 1;
        char flag = 'M';
        //#######################################################################################################################################################################
        for(i = 0; i < LIMIT; i++) {
            if(i == LIMIT - 1)
                flag = 'L';
            if (sendto(s, addHeader(fp[i].payload, i + 1, 10000, flag, fragmentSize), fp[i].len, 0, (struct sockaddr *) &server, sizeof(server)) == SOCKET_ERROR) {
                printf("Error: 'socket error'");
                exit(EXIT_FAILURE);
            }
        }
        memset(msg, 0, strlen(msg));
        timeoutClient(s, 1);
        if(recv(s, msg, BUFFLEN,0) == SOCKET_ERROR){
            printf("Error: 'Socket error'");
            exit(EXIT_FAILURE);
        }
        //printf("\nServer response: %s", msg);
        print(j, msg, HEAD);
        //memset(msg, 0, strlen(msg));
        memset(filename, 0, strlen(filename));
    }
    myFree(fp);
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
