
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>

#define MAXLINE 1000 
char cBuffer[MAXLINE];
char kBuffer[MAXLINE];
char dBuffer[MAXLINE];
void doTheThing(int clientFD);

/******************************/
void error(const char* msg, int e) {perror(msg); exit(e);}

/***************************/
int main(int argc, char *argv[]){

    memset(cBuffer, '\0', MAXLINE);
    memset(kBuffer, '\0', MAXLINE);
    memset(dBuffer, '\0', MAXLINE);

    int childFD[5] = {0};
    int children = 0; 
    pid_t pidChild[5];


    int listenSocketFD, portNumber, charRead, charWrite; 
    struct sockaddr_in serverAddy, clientAddy;
    socklen_t sizeOfClient;


    if(argc < 2) {
        fprintf(stderr, "USAGE %s port\n", argv[0]);
        exit(1);
    }

    memset((char *)&serverAddy, '\0',sizeof(serverAddy));
    portNumber = atoi(argv[1]);
    serverAddy.sin_family = AF_INET;
    serverAddy.sin_port = htons(portNumber);
    serverAddy.sin_addr.s_addr = INADDR_ANY;

    listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    if(listenSocketFD < 0) error("ERROR opening socket", 1);
    if(bind(listenSocketFD, (struct sockaddr *)&serverAddy, sizeof(serverAddy)) <0){
    error("ERROR on binding",1);}
    listen(listenSocketFD, 5);
 
    while(1){
        if(children <5){
            int exitMethod = -5; 
            int i =0;
            int clientFD;
            sizeOfClient = sizeof(clientAddy);
            childFD[children]= accept(listenSocketFD, (struct sockaddr *)&clientAddy, &sizeOfClient); 
            if(childFD[children]<0) error("ERROR on accept",1);
            clientFD = childFD[children]; 
            pidChild[children] = fork();
            
        switch(pidChild[children]){
            case -1: 
                fprintf(stderr, "fork failed.\n");
                exit(1);
                break;
            
            case 0: 
                charRead = recv(clientFD, dBuffer, MAXLINE-1, 0);
                if(charRead <0) error("ERROR reading from socket",1);
                //printf("we have recieved our first snippet in the child: %s", eBuffer);
                //fflush(stdout);
                if(strcmp(dBuffer, "decrypt")==0){
                    charWrite = send(clientFD, dBuffer, strlen(dBuffer), 0);
                    if(charWrite <0) error("ERROR writing to socket",1);     
                    memset(dBuffer, '\0', MAXLINE);
                    strcpy(dBuffer, "2");
                    //printf("we are doing the thing in the server\n");
                    //fflush(stdout);
                    doTheThing(clientFD);
                    exit(0);
                }    
                else{
                  error("I was made only for the decryption client",1);  
                }
            default: 
                i = children;
                children++;
                pidChild[i] = waitpid(-1, &exitMethod, WNOHANG);
                while(pidChild[i]>0){
                    if(WIFEXITED(exitMethod)){
                        close(childFD[i]);
                        childFD[i] = 0;
                        pidChild[i] = 0;
                        children--;
                    }
                    else if(WIFSIGNALED(exitMethod)){
                        close(childFD[i]);
                        childFD[i] = 0;
                        pidChild[i] = 0;
                        children--;
                    }    
                    pidChild[i] = waitpid(-1, &exitMethod, WNOHANG);
                }

            }
       
      }
    }
    close(listenSocketFD);
    return 0;
}

/********************************/
void doTheThing(int clientFD){
    int charRead, charWrite;
    int i;
    char temp;
    int stopLoop = 0;
    int flag = 0; 
    int kBufferGood = 0; 
    int cBufferGood = 0; 
    int gotTheStrings = 0;
    
    do{
    gotTheStrings = 0; 
    kBufferGood = 0; 
    cBufferGood = 0; 
    while(!gotTheStrings){
    charRead = recv(clientFD, dBuffer, MAXLINE-1, 0);
    if(charRead <0) error("ERROR reading from socket",1);
        if(strcmp(dBuffer, "1") == 0){
            charWrite = send(clientFD, dBuffer, strlen(dBuffer), 0);
            if(charWrite <0) error("ERROR writing to socket",1);     
            memset(dBuffer, '\0', MAXLINE);
            charRead = recv(clientFD, kBuffer, MAXLINE-1, 0);
            if(charRead <0) error("ERROR reading from socket",1);
            kBufferGood = 1; 
        }
        else if(strcmp(dBuffer, "0")==0){
            charWrite = send(clientFD, dBuffer, strlen(dBuffer), 0);
            if(charWrite <0) error("ERROR writing to socket",1);     
            memset(dBuffer, '\0', MAXLINE);
            charRead = recv(clientFD, cBuffer, MAXLINE-1, 0);
            if(charRead <0) error("ERROR reading from socket",1);
            cBufferGood = 1;
        }
        if(cBufferGood && kBufferGood){
            gotTheStrings = 1;
        }    
    }
    i =0;
    while(cBuffer[i]<MAXLINE-1 && cBuffer[i] != '\0'){
    //printf("cBuffer[%d] is %c, %d and cBuffer[%d] is %c, %d\n", i, kBuffer[i],dBuffer[i], i, cBuffer[i], cBuffer[i]);
    //fflush(stdout);

        if(kBuffer[i] == 65){
            kBuffer[i] = 26;
        }

        else if(kBuffer[i] == 32){
            kBuffer[i] = 27;
        }
        else if(kBuffer[i] > 65 && kBuffer[i] <91){
            kBuffer[i] = kBuffer[i]- 65;
        }
        //cBuffer 
        if(cBuffer[i] == 32){
            cBuffer[i] = 27;
        }
        else if(cBuffer[i] == 65){
            cBuffer[i] = 26;
        }
        else if(cBuffer[i] > 65 && cBuffer[i] <91){
            cBuffer[i] = cBuffer[i]- 65;
        }
        else if(cBuffer[i] == 10){
            flag = 1; 
            cBuffer[i] = '\0';
        }    

           /* if(i<100){
            printf("k and c %d after is: %d and %d\n", i, kBuffer[i], cBuffer[i]);
            fflush(stdout);
            }*/
        i++;        
    }
    i =0;
    while(cBuffer[i]< MAXLINE-1 && cBuffer[i] != '\0'){
        temp = cBuffer[i] - kBuffer[i];//we reversed the order and sign
        /*if(i<100){
        printf("temp %d is: %d \n",i, temp);
        fflush(stdout);
        }*/
       //OLD if (temp > 27){
       if(temp <=0){
           temp = temp + 27;
        }    
        if(temp == 26){
            temp = 65;
        }    
        else if(temp == 27){
           temp = 32; 
        }else{
           temp = temp + 65; 
        }
       
        dBuffer[i] = temp;
        temp = '\0';
        /*if(i<100){
        printf("dBuffer[%d] is: %c \n", i, dBuffer[i]);
        }*/
        i++;
   }
    charWrite = send(clientFD, dBuffer, strlen(dBuffer), 0);
    if(charWrite <0) error("ERROR writing to socket",1);                
    memset(dBuffer, '\0', MAXLINE);
    memset(kBuffer, '\0', MAXLINE);
    memset(cBuffer, '\0', MAXLINE);
    if(flag){
        stopLoop = 1; 
    }

    }while(!stopLoop);
    
    
}
