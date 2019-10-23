#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <winsock2.h>
//#include <pthread.h>

#define iplen 15
#define HEAD 9
#define BUFFLEN 3000000 //for 2MB file
#define DEFAULTSIZE 10
#define MAXFRAGMENT 1472 - HEAD
#define NAMESIZE 100
#define LIMIT 10


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
    int len = window + HEAD;
    u_char *msg = (u_char*)malloc((len + 1) * sizeof(u_char));
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


int count(int len){                                                                                  //count how many fragments will the message be split into
    double x = (double)len / (double)fragmentSize;
    int ret = (int)ceil(x);
    printf("Message will be split into %d fragments", ret);
    return ret;
}


//TODO - opravit bugy, ked to pada a tak >?dajmetomu?
//TODO - checksum vypocet + overenie
//TODO - simulovanie chyby
//TODO - absolutna cesta a meno suboru
//TODO - spatne odosielanie chybnych fragmentov - nejako si pamatat ich indexy
//TODO - init pri posielani spravy - poslem maximalnu velkost fragmentu + pocet sprav nech ich viem vypisat na druhej strane
//TODO - init pri posielani suboru - poslem velkost suboru + jeho meno, taktiez musim aj tak poslat pocet fragmentov + velkost max fragmentu :)
//TODO - urobit ukladanie suboru na strane servera
//TODO - keepalive na strane klienta, ked sa nekorektne odpoji tak server timne out.
//TODO - napisat dokumentaciu

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


int verify(u_char *fragment, short checksum){
    return 1;
}


FRAGMENT *fragment(u_char *msg, int len, char flag){                                                                      //fragmenting the message
    printf("\nFRAGMENTING BEGINS");
    int i, j, curr = 0;
    fragmentCount = count(len);
    printf("\nTEST 0");
    FRAGMENT *str = (FRAGMENT*)malloc(fragmentCount * sizeof(FRAGMENT));
    printf("\nTEST 1");
    for(i = 0; i < fragmentCount; i++){
        str[i].payload = (u_char*)calloc(fragmentSize, sizeof(u_char));
    }
    printf("\nTEST 2");/*
    for(i = 0; i < fragmentCount; i++){
        for(j = 0; j < fragmentSize; j++){
            str[i].payload[j] = msg[curr++];
        }
        str[i].payload[j] = '\0';
        //printf("\nFRAGMENT %d: %s", i+1, str[i].payload);
    }*/
    //u_char *tmp;
    printf("\nTEST 3");
    for(i = 0; i < fragmentCount; i++){
        //tmp = str[i].payload;
        str[i].len = HEAD + fragmentSize;
        if(i == fragmentCount - 1) {
            flag = 'L';
            if(len % fragmentSize != 0)
                str[i].len = HEAD + (len % fragmentSize); //??? this should theoretically work
        }
        str[i].payload = addHeader(msg + i * fragmentSize, i + 1, 1000, flag, str[i].len - HEAD);
        //free(tmp);
        print(i + 1, str[i].payload, str[i].len);
    }
    printf("\nTEST 4");
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


char *openFile(char *path, int *len) {                            //open binary file and load its content into a buffer(max 2MB)
    //TODO - bug, file not transfering correctly, worked in previous versions of program
    //TODO - size n is correct, but buffer is only 8B? what
    size_t n;
    FILE *f = fopen(path, "rb");
    if(f == NULL){
        printf("Error: 'file not found'");
        exit(EXIT_FAILURE);
    }
    fseek(f, 0L, SEEK_END);
    n = ftell(f);
    *len = n;
    rewind(f);
    u_char *buffer = malloc(n * sizeof(u_char));
    fread(buffer, n + 1, 1, f);
    printf("\nSize of file is %d B\n", n);
    fclose(f);
    //print(99, buffer, n);
    //copyFile("LANtransfer.png", buffer, n);
    return buffer;
}


int toInt(u_char *data, int index){
    return data[index] * pow(256, 3) + data[index + 1] * pow(256,2) + data[index + 2] * 256 + data[index + 3];
}


short toShort(u_char *data, int index){
    return data[index] * 256 + data[index + 1];
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
            printf("ACK\n");
            return 3;
        case('N'):
            printf("NACK\n");
            return 4;
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


void handshakeClientFile(SOCKET s, struct sockaddr_in server, char *filename, int len){
    char msg[HEAD];
    if(sendto(s, addHeader(filename, len, 0, 'F', strlen(filename)), HEAD, 0, (struct sockaddr *) &server, sizeof(server)) == SOCKET_ERROR) {
        printf("Error: 'socket error'");
        exit(EXIT_FAILURE);
    }
    timeoutClient(s, 1);
    if(recv(s, msg, HEAD,0) == SOCKET_ERROR){
        printf("Error: 'Socket error'");
        exit(EXIT_FAILURE);
    }
}


char *handshakeServerFile(SOCKET s, struct sockaddr_in server, struct sockaddr_in client, int *ret){
    int size = NAMESIZE + HEAD;
    int len = sizeof(client);
    char *msg = malloc(size * sizeof(char));
    if(recvfrom(s, msg, size,0, (struct sockaddr *) &client, &len) == SOCKET_ERROR){
        printf("Error: 'Socket error'");
        exit(EXIT_FAILURE);
    }
    *ret = toInt(msg, 0);
    if(sendto(s, addHeader("",0,0,'A',0), HEAD, 0, (struct sockaddr *) &client, sizeof(client)) == SOCKET_ERROR) {
        printf("Error: 'socket error'");
        exit(EXIT_FAILURE);
    }
    return decodeMessage(msg, (int)(toShort(msg, 6) + HEAD));
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


int comp (const void * a, const void * b){
    ACCEPTED *x = (ACCEPTED*) a;
    ACCEPTED *y = (ACCEPTED*) b;
    return x->seq - y->seq;
}


ACCEPTED* order(ACCEPTED *str, int len){
    qsort(str, len, sizeof(ACCEPTED), comp);
    return str;
}


void reconstructMessage(ACCEPTED *str, int len){
    str = order(str, len);
    char *msg = malloc(BUFFLEN * sizeof(char));
    int i;
    for(i = 0; i < len; i++) {
        strcat(msg, decodeMessage(str[i].payload, str[i].len)); //str[i].len?
    }
    printf("\nFinal message: %s\n", msg);
    free(msg);
}


char convertFlag(char a){
    if(a == 'A')
        return '1';
    else
        return '0';
}


u_char *response(ACCEPTED *str, int prev, int len){
    u_char *msg = malloc((len - prev) * sizeof(u_char));
    int i, j = 0;
    for(i = prev; i < len; i++){
        msg[j++] = convertFlag(str[i].payload[8]);
    }
    return addHeader(msg, prev, 0, 'R', len - prev);
}


void printACK(u_char *msg, int len){
    int i, start = toInt(msg, 0);
    for(i = HEAD; i < len; i++){
       if((int)msg[i] == 48)
           printf("\n%d. Fragment - ACK", start + i - HEAD + 1);
       else
           printf("\n%d. Fragment - NACK", start + i - HEAD + 1);
    }
}

ACCEPTED setAccepted(ACCEPTED a, int recvb){
    a.payload = realloc(a.payload, recvb * sizeof(u_char));
    a.seq = toInt(a.payload, 0);
    a.len = recvb;
    print(a.seq, a.payload, a.len);
    return a;
}


void server(int port){                                                                                //server side code, listening on specified port number until connection is established
    initializeWinsock();
    SOCKET s = createSocket(s);
    u_char *msg = malloc(MAXFRAGMENT * sizeof(u_char));
    ACCEPTED *accepted = NULL;
    u_char *resp;

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

    handshakeServer(s, server, client);                                                                   //server waits for SYN, responds with ACK - connection is established
    int recvb, i, j, prev, state;
    while(1){
        memset(msg, 0, MAXFRAGMENT);
        i = 0;                              //number of fragments received
        prev = 0;                           //last fragment sequence confirmed
        state = 1;                          //decides if specific message is currently being sent or not
        accepted = (ACCEPTED*)malloc(sizeof(ACCEPTED));
        while(state) {
            j = 0;
            while(j++ < LIMIT) {
                accepted = realloc(accepted, (i + 1) * sizeof(ACCEPTED));
                accepted[i].payload = (u_char*)calloc(MAXFRAGMENT + HEAD, sizeof(u_char));
                timeoutServer(s);
                if ((recvb = recvfrom(s, accepted[i].payload, MAXFRAGMENT + HEAD, 0, (struct sockaddr *) &client, &len)) == SOCKET_ERROR) {
                    printf("Error: 'Socket error'");
                    exit(EXIT_FAILURE);
                }
                printf("\nReceived %d B", recvb);
                accepted[i] = setAccepted(accepted[i], recvb);
                switch (handleFlag(accepted[i++].payload[8])) {
                    case 1:                                                                 //end of file/message transfer
                        state = 0;
                        //TODO - implement resend of faulty fragments here
                        goto rsp;
                    case 2:                                                                         //end of communication
                        if (sendto(s, addHeader("", 2, 0, 'A', 0), HEAD, 0, (struct sockaddr *) &client, sizeof(client)) == SOCKET_ERROR) {
                            printf("Error: 'socket error'");
                            exit(EXIT_FAILURE);
                        }
                        free(msg);
                        closeSocket(s);
                        return;
                }
            }
            rsp:
            resp = response(accepted, prev, i);
            printf("\nSending ACK\n");
            if (sendto(s, resp, HEAD + i - prev, 0, (struct sockaddr *) &client, sizeof(client)) == SOCKET_ERROR) {
                printf("Error: 'socket error'");
                exit(EXIT_FAILURE);
            }
            free(resp);
            prev = i;
        }
        reconstructMessage(accepted, i);
        printf("Accepted %d fragments", i);
        if(accepted != NULL){
            int count = i;
            for(i = 0; i < count; i++){
                free(accepted[i].payload);
                accepted[i].seq = 0;
                accepted[i].len = 0;
            }
            free(accepted);
        }
    }
}


void client(char *ip, int port){
    initializeWinsock();
    SOCKET s = createSocket(s);
    //pthread_t tID;                                                                              //Posix thread for keepalive prob.
    FRAGMENT *p = NULL;
    u_char *msg = malloc(BUFFLEN * sizeof(u_char));
    char *filename = malloc(NAMESIZE * sizeof(char));
    int len, k;

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
        if(p != NULL){                                                                           //my own free
            printf("\nTEST 5");
            printf("\nTEST 5,5 - FREEING BEGINS");
            for(k = 0; k < fragmentCount; k++){
                free(p[k].payload);
            }
            printf("\nTEST 6");
            free(p);
            printf("\nTEST 7");
            p = NULL;
        }
        printf("\n>>>");
        fflush(stdin);                                                                           //flush the standart input
        gets(msg);
        len = strlen(msg);
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
            printf("Full path: \n");
            msg = openFile(filename, &len);
        }
        else if(strcmp(msg, ":error") == 0){
            printf("Error msg will be simulated here\n");
        }
        else if(strcmp(msg, ":clear") == 0) {
            system("cls");
            continue;
        }//prompt - also contains file!
        //##################################
        p = fragment(msg, len, 'M');
        memset(msg, 0, len);        //toto moze byt blbost pri subore!
        //##################################
        int i = 0, j, recvb;
        while(i < fragmentCount){
            j = 0;
            while(j++ < LIMIT){
                if (sendto(s, p[i].payload, p[i].len, 0, (struct sockaddr *) &server, sizeof(server)) == SOCKET_ERROR) {
                    printf("Error: 'socket error'");
                    exit(EXIT_FAILURE);
                }
                i++;
                if(p[i - 1].payload[8] == 'L')
                    break;
            }
            timeoutClient(s, 1);
            if ((recvb = recv(s, msg, MAXFRAGMENT, 0)) == SOCKET_ERROR) {
                printf("Error: 'Socket error'");
                exit(EXIT_FAILURE);
            }
            printf("\nServer response: ");
            print(i, msg, recvb);
            printACK(msg, recvb);
            memset(msg, 0, recvb);
            memset(filename, 0, strlen(filename));
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
