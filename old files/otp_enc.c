//connects to otp_enc_d, and asks it to perform a one time pad
//syntax ---> otp_enc plaintext key port.
//plaintext is the name of a file in the current directory that contains the text you want to encrypt. key contains the encryption key you wish 
//to use. and Port is the port that otp_enc should attempt to connect to otp_enc_d on.
//when it recieves text back from otp_enc_d, should output to stdout
//if receives any key or plaintext files with ANY bad characters
//in them, or the key file is shorter that the plaintext, then 
//it should terminate, send err to stderr and set exit to 1
//should NOT be able to connect to otp_dec_d, even if it tries to connect on the correct port - the programs need to reject each other
//this program reports the rejection to stderr and then terminates
//if otp_enc cannot connect to the otp_enc_d server for any reason, it 
//shoudl report err to stderr and exit value 2.

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
    strcpy(buffer, "encrypt");
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
     if(strcmp(buffer, "encrypt") == 0){
        memset(buffer, '\0', sizeof(buffer));
        doTheThing(socketFD, argv[1], argv[2]); 
     }
     else{
    //reject the otp_dec_d server connection  
        close(socketFD);
        fprintf(stderr,"I was only made for the encryption server, its not on port %d\n", argv[22]);
        exit(2);
     }

    close(socketFD);
    return 0;
}  
/*******************/
void doTheThing(int socketFD, char* p, char* k){
    FILE* kFile;
    FILE* pFile;
    char c;
    int pCount = 0; 
    int kCount = 0;   
    int checkSend = -5;
    int stopLoop = 0;
    int charWrite, charRead;
    int flag = 0; 

    //printf("in client do the thing \n");
    //fflush(stdout);

    pFile = fopen(p, "r");
    if(pFile == NULL){
        fclose(pFile);
        error("plain text file didn't load successfully\n",1);
    }
    while((c=fgetc(pFile)) != EOF){
        if(c == 32 || (c>64 && c<91)){
                pCount++;       //space and capital letters
        }        
        else if(c == 10){
                pCount++;       //newline
        }
        else{
          fclose(pFile);
          error("invalid characters in plain text file",1);
        }
    }
    c = '\0';
    fclose(pFile);

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
        else{
          fclose(kFile);
          error("invalid characters in key file",1);
        }

    }
    if(kCount < pCount){
        fclose(kFile);
        error("key file is shorter than plain text file\n",1);
    }    
    fclose(kFile);

   // printf("now we are happy with the files\n");
   // fflush(stdout);

    kFile = fopen(k, "r");
    pFile = fopen(p, "r");
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
        strcpy(buffer, "0");  //let them know we are sending the plain file 
        //printf("sending the 0\n");
        charWrite = send(socketFD, buffer, strlen(buffer), 0);  //send
        memset(buffer, '\0', sizeof(buffer)); //clear buffer
        charRead = recv(socketFD, buffer, MAXLINE-1,0); //recv
        if(charRead <0) error("CLIENT: Error receiving",0);
        memset(buffer, '\0', sizeof(buffer));  //clear buffer
        //now send 1000 chars of plain file 
        if(fgets(buffer, MAXLINE, pFile)!=NULL){ 
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
    fclose(pFile);
 
}
