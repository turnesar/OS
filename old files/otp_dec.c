
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/ioctl.h>

#define MAXLINE 1000

char buffer[MAXLINE];
void doTheThing(int socketFD, char* p, char* k);

///
void error(const char *msg, int e) {perror(msg), exit(e);}
///
int main(int argc, char *argv[]){

    memset(buffer, '\0', sizeof(buffer));
    int socketFD, portNumber, charWrite, charRead;
    struct sockaddr_in serverAddy;
    struct hostent* serverHostInfo;
    strcpy(buffer, "decrypt");
    if(argc < 3) { error("port in use", 0);}
    //need to double check arguments, port is the third argument.
    
    memset((char*)&serverAddy, '\0', sizeof(serverAddy));
    portNumber = atoi(argv[3]);
    serverAddy.sin_family = AF_INET;
    serverAddy.sin_port = htons(portNumber);
    serverHostInfo = gethostbyname("localhost");
    if(serverHostInfo == NULL){
        error("CLIENT: error, no such host\n", 2);
    }
    memcpy((char*)serverHostInfo->h_addr, (char*)&serverAddy.sin_addr.s_addr, serverHostInfo->h_length);

    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if(socketFD <0) error("CLIENT: ERROR opening socket",2);
    //printf("socket opened\n"); 
    //fflush(stdout);
    if(connect(socketFD, (struct sockaddr*)&serverAddy, sizeof(serverAddy))<0){
    error("CLIENT: error Connecting",2);}
    //printf("client connected\n");
    //fflush(stdout);
    charWrite = send(socketFD, buffer, strlen(buffer), 0);
    //printf("sending our first packet\n");
    //fflush(stdout);
    if(charWrite <0) error("CLIENT: ERROR authenticating",0);

    int checkSend = -5;
    do{
        ioctl(socketFD, TIOCOUTQ, &checkSend);
     }while (checkSend >0);
     if(checkSend < 0) error("ioctl error", 0);
     memset(buffer, '\0', sizeof(buffer));
     charRead = recv(socketFD, buffer, MAXLINE-1,0);
     if(charRead <0) error("CLIENT: Error authenticating", 1);
     if(strcmp(buffer, "decrypt") == 0){
        memset(buffer, '\0', sizeof(buffer));
        doTheThing(socketFD, argv[1], argv[2]); 
     }
     else{
    //reject the otp_dec_d server connection  
        close(socketFD);
        fprintf(stderr, "I was only made for the encryption server, that isn't on port %d\n", argv[2]);
        exit(2);
     }

    close(socketFD);
    return 0;
}  
/*******************/
void doTheThing(int socketFD, char* cipher, char* k){
    FILE* kFile;
    FILE* cFile;
    char c;
    int cCount = 0; 
    int kCount = 0;   
    int checkSend = -5;
    int stopLoop = 0;
    int charWrite, charRead;
    int flag = 0; 

    //printf("in client do the thing \n");
    //fflush(stdout);

    cFile = fopen(cipher, "r");
    if(cFile == NULL){
        fclose(cFile);
        error("plain text file didn't load successfully\n",1);
    }
    while((c=fgetc(cFile)) != EOF){
        if(c == 32 || (c>64 && c<91)){
                cCount++;       //space and capital letters
        }        
        else if(c == 10){
                cCount++;       //newline
        }
    }
    c = '\0';
    fclose(cFile);

    kFile = fopen(k, "r");
    if(kFile == NULL){
        fclose(kFile);
        error("key file didn't load successfully\n", 1);
    }
    while((c=fgetc(kFile)) != EOF){
        if(c == 32 || (c>64 && c<91)){
                kCount++;       //space and capital letters
        }        
        else if(c == 10){
                kCount++;        //newline
        }

    }
    if(kCount < cCount){
        fclose(kFile);
        error("key file is shorter than cipher text file\n",1);
    }    
    fclose(kFile);

   // printf("now we are happy with the files\n");
   // fflush(stdout);

    kFile = fopen(k, "r");
    cFile = fopen(cipher, "r");
    do{
        //printf("sending the 1\n");
        strcpy(buffer, "1");  //let them know we are sending the key first 
        charWrite = send(socketFD, buffer, strlen(buffer), 0);  //send
        memset(buffer, '\0', sizeof(buffer)); //clear buffer
        charRead = recv(socketFD, buffer, MAXLINE-1,0); //recv
        if(charRead <0) error("CLIENT: Error receiving",0);
        memset(buffer, '\0', sizeof(buffer));  //clear buffer
        //now send 1000 chars of key file 
        if(fgets(buffer, MAXLINE, kFile)!=NULL){ 
            //printf("\nsending the key buffer: %s\n", buffer);
            charWrite = send(socketFD, buffer, strlen(buffer), 0);
            if(charWrite <0) error("CLIENT: ERROR sending",0);
            checkSend = -5;
            do{
                ioctl(socketFD, TIOCOUTQ, &checkSend);
            }while (checkSend >0);
            if(checkSend < 0) error("ioctl error", 0);
            memset(buffer, '\0', sizeof(buffer));
        }
        else{
            //stopLoop = 1;
        }
        strcpy(buffer, "0");  //let them know we are sending the cipher file 
        //printf("sending the 0\n");
        charWrite = send(socketFD, buffer, strlen(buffer), 0);  //send
        memset(buffer, '\0', sizeof(buffer)); //clear buffer
        charRead = recv(socketFD, buffer, MAXLINE-1,0); //recv
        if(charRead <0) error("CLIENT: Error receiving",0);
        memset(buffer, '\0', sizeof(buffer));  //clear buffer
        //now send 1000 chars of plain file 
        if(fgets(buffer, MAXLINE, cFile)!=NULL){ 
           // printf("\nsending the plain buffer: %s\n", buffer);
            if(buffer[strlen(buffer)-1] == '\n'){
                flag = 1; 
            }    
            charWrite = send(socketFD, buffer, strlen(buffer), 0);
            if(charWrite <0) error("CLIENT: ERROR sending",0);
            checkSend = -5;
            do{
                ioctl(socketFD, TIOCOUTQ, &checkSend);
            }while (checkSend >0);
            if(checkSend < 0) error("ioctl error", 0);
            memset(buffer, '\0', sizeof(buffer));
            charRead = recv(socketFD, buffer, MAXLINE-1,0); //recv
            if(charRead <0) error("CLIENT: Error receiving",0);
            printf("%s",buffer);
            fflush(stdout);
            memset(buffer, '\0', sizeof(buffer));  //clear buffer
        }
        if(flag){
            stopLoop = 1;
            printf("\n");
            fflush(stdout);
        }    
    }while(!stopLoop);

    fclose(kFile);
    fclose(cFile);
 
}
