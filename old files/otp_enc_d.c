/******************************************************
 *Name: Sarah Turner 
 *PROJECT 4, CS344
 *DUE JUNE 9, 2019i
 *requested syntax: otp_enc_d <listening port> &
 *Description: this is the encryption server.  Upon 
 *starting the server in the background, it will set up 
 *the socket and listen for clients.  Upon a client 
 *request it will accept the request if there are less
 *than 5 clients currently.  Upon acceptance, the child
 *process will do a 'handshake' by passing a verification
 *word back and forth. After verification, the child
 *process calls a separate function that will loop to  
 *recieve the key and plaintext file from the server, 
 *perform OTP calculations and then send an encrypted
 *buffer back.  The server assumes verified length
 *of key file and only capital letters, spaces, or
 *a newline as a terminator.  Encrtyption stops 
 *and client is closed when newline is encountered.
 *
 *References: all lectures,and the OTP wiki page.  My 
 *own smallsh program for the waitpid logic.
 * *************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>

#define MAXLINE 1000            //define our buffer length
char pBuffer[MAXLINE];          //used for plaintext buffer
char kBuffer[MAXLINE];          //used for key buffer 
char eBuffer[MAXLINE];          //used for encrypted test buffer 
void doTheThing(int clientFD);  //defining function 

/******************************
 *Name: void error(const char* msg, int e)
 *Description: takes a msg and exit number. Prints
 *message to stderr and then exits according to 
 *the defined number
 *references: lectures and example client/server 
 * *****************************/
void error(const char* msg, int e) {fprintf(stderr,"%s\n",msg); exit(e);}

/**********************************
 *Name: main(int argc, char *argv[])
 *Description: desired syntax is otp_enc_d <listening port> & 
 *Main sets up the socket for the server connection
 *and then once the socket is open loops the client 
 *fork process while the program is running.  
 *Forks clients and calls the separate encryption function
 * **************************/
int main(int argc, char *argv[]){

    memset(pBuffer, '\0', MAXLINE);     //clear out our global buffers
    memset(kBuffer, '\0', MAXLINE);
    memset(eBuffer, '\0', MAXLINE);

    int childFD[5] = {0};               //used to track our 5 clientFDs
    int children = 0;                   //used to track how many clients we have, and as index 
    pid_t pidChild[5];                  //used to track 6 client pids


    int listenSocketFD, portNumber, charRead, charWrite;   //setting up our socket and verifying send/rcv
    struct sockaddr_in serverAddy, clientAddy;             //server and client setup 
    socklen_t sizeOfClient;                                //client size used for setup 


    if(argc < 2) {                                          //straight from server.c 
        fprintf(stderr, "USAGE %s port\n", argv[0]);
        exit(1);
    }

    memset((char *)&serverAddy, '\0',sizeof(serverAddy));    //also straight from server.c, initializing our 
    portNumber = atoi(argv[1]);                              //server struct with the port number passed in  
    serverAddy.sin_family = AF_INET;                         //and setting default parameters
    serverAddy.sin_port = htons(portNumber);
    serverAddy.sin_addr.s_addr = INADDR_ANY;

    listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);       //setting our socket 
    if(listenSocketFD < 0) error("ERROR opening socket", 1);
    if(bind(listenSocketFD, (struct sockaddr *)&serverAddy, sizeof(serverAddy)) <0){    //binding to socket
    error("ERROR on binding",1);}
    listen(listenSocketFD, 5);

    //now our socket is good. Now on to the clients!
 
    while(1){
        if(children <5){                                    //we only make kids if we have <5 kids, good life rule too
            int exitMethod = -5;                            //for our exit on waitpid  
            int i =0;                                       //used as index
            int clientFD;                                   //used for each client to simplify variable

            sizeOfClient = sizeof(clientAddy);              //setting up our client and adding them to our childFD array 
            childFD[children]= accept(listenSocketFD, (struct sockaddr *)&clientAddy, &sizeOfClient); 
            if(childFD[children]<0) error("ERROR on accept",1);
            clientFD = childFD[children];                   //this just makes it easier 
            pidChild[children] = fork();                    //now we fork to make da baby 
            
        switch(pidChild[children]){                         
            case -1:                                        //no baby :(
                fprintf(stderr, "fork failed.\n");
                exit(1);
                break;
            
            case 0: 
                charRead = recv(clientFD, eBuffer, MAXLINE-1, 0);       //we recieve a handshake message from our client
                if(charRead <0) error("ERROR reading from socket",1);
                //printf("we have recieved our first snippet in the child: %s", eBuffer);
                //fflush(stdout);
                if(strcmp(eBuffer, "encrypt")==0){                      //we verify this is the client we want
                    charWrite = send(clientFD, eBuffer, strlen(eBuffer), 0);
                    if(charWrite <0) error("ERROR writing to socket",1);     
                    memset(eBuffer, '\0', MAXLINE);
                    strcpy(eBuffer, "2");                               //initialize for the follow on function 
                    //printf("we are doing the thing in the server\n");
                    //fflush(stdout);
                    doTheThing(clientFD);                               //this does the encryption 
                    exit(0);
                }    
                else{
                  exit(2);                                              //this is not the server you are looking for 
                }
            default:                                                                                        
                i = children;                                           //back in the parent we track the kiddos          
                children++;                                             
                pidChild[i] = waitpid(-1, &exitMethod, WNOHANG);        //watch as our forked processes end 
                while(pidChild[i]>0){
                    if(WIFEXITED(exitMethod)){
                        close(childFD[i]);                              //close as needed, reset our arrays, and count 
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

            }                   //closes the switch 
      }                         //closes the if(children <5)
    }                           //closes the while(1)
    close(listenSocketFD);      //pretty sure this does nothing here. You have to kill it because of the while(1)
                                //but for solidarity...
    return 0;
}

/******************************
 *Name: void doTheThing(int clientFD)
 *Description: Recieves the clientFD
 *from the child process.  It first recieves 
 *a 1 or 0 from the client to tell them which 
 *buffer (key or cipher respectively) to fill. 
 *It takes 1000 chars at a time from key 
 *and plaintext and then sends back the 
 *cipher after calculating the conversion.
 *Upon getting a newline from the buffer, 
 *it stops the looping.
 *references: that sweet wiki page on OTP
 * ******************************/
void doTheThing(int clientFD){
    int charRead, charWrite;        //verify send/recv
    int i;                          //looping char indexes 
    char temp;                      //temp char while we do math for encryption 
    int stopLoop = 0;               //our loop while condition 
    int flag = 0;                   //we set the flag to let them know to stoploop at the end
    int kBufferGood = 0;            //these two ints are flags to show that 
    int pBufferGood = 0;            //the two buffers are ready for the math
    int gotTheStrings = 0;          //used to loop to fill the buffers 
    
    do{
    gotTheStrings = 0;              //reseting our variables
    kBufferGood = 0; 
    pBufferGood = 0; 
    //this fills the key and plaintext buffers 
    while(!gotTheStrings){
    charRead = recv(clientFD, eBuffer, MAXLINE-1, 0);  
    if(charRead <0) error("ERROR reading from socket",1);
        if(strcmp(eBuffer, "1") == 0){                          //you will recv the key next 
            charWrite = send(clientFD, eBuffer, strlen(eBuffer), 0);//telling them to send it
            if(charWrite <0) error("ERROR writing to socket",1);     
            memset(eBuffer, '\0', MAXLINE);                     
            charRead = recv(clientFD, kBuffer, MAXLINE-1, 0);    //have the key buffer now        
            if(charRead <0) error("ERROR reading from socket",1);
            kBufferGood = 1;                                    //set the kbuffer is good flag
        }
        else if(strcmp(eBuffer, "0")==0){                       //you will recv the plaintext next
            charWrite = send(clientFD, eBuffer, strlen(eBuffer), 0);//telling them to send it 
            if(charWrite <0) error("ERROR writing to socket",1);     
            memset(eBuffer, '\0', MAXLINE);
            charRead = recv(clientFD, pBuffer, MAXLINE-1, 0);   //have the plaintext buffer now 
            if(charRead <0) error("ERROR reading from socket",1);
            pBufferGood = 1;                                    //set the pbuffer is good flag
        }
        if(pBufferGood && kBufferGood){                         //if we have both buffers...
            gotTheStrings = 1;                                  //exit the loop
        }    
    }

    i =0;           //reset our index 

    //this translates the key and plaintext to the cipher
    while(pBuffer[i]<MAXLINE-1 && pBuffer[i] != '\0'){          
    //printf("kBuffer[%d] is %c, %d and pBuffer[%d] is %c, %d\n", i, kBuffer[i],kBuffer[i], i, pBuffer[i], pBuffer[i]);
    //fflush(stdout);

    //so we take each part of both buffers and do some math to get them to 1-27 ascii  
        if(kBuffer[i] == 65){           //hey if it is A make it 26, it didn't like 0
            kBuffer[i] = 26;
        }

        else if(kBuffer[i] == 32){      //a space becomes 27
            kBuffer[i] = 27;
        }
        else if(kBuffer[i] > 65 && kBuffer[i] <91){  //all other letters become 1-25
            kBuffer[i] = kBuffer[i]- 65;
        }
        //pBuffer 
        if(pBuffer[i] == 32){            //we repeat this process in the pBuffer    
            pBuffer[i] = 27;
        }
        else if(pBuffer[i] == 65){
            pBuffer[i] = 26;
        }
        else if(pBuffer[i] > 65 && pBuffer[i] <91){
            pBuffer[i] = pBuffer[i]- 65;
        }
        else if(pBuffer[i] == 10){      //if it is a newline..
            flag = 1;                   //set our flag 
            pBuffer[i] = '\0';
        }    

           /* if(i<100){
            printf("k and p %d after is: %d and %d\n", i, kBuffer[i], pBuffer[i]);
            fflush(stdout);
            }*/
        i++;                            //increment
    }

    i =0;                               //reset variables

    //now we go back through and convert both buffers into the encrypted buffer 
    while(pBuffer[i]< MAXLINE-1 && pBuffer[i] != '\0'){
        temp = kBuffer[i] + pBuffer[i];         //our placeholder
        /*if(i<100){
        printf("temp %d is: %d \n",i, temp);
        fflush(stdout);
        }*/
        if (temp > 27){
           temp = temp % 27;
        }    
        if(temp == 26){             //if 26, that is A 
            temp = 65;
        }    
        else if(temp == 27){        //if 27, that is a space
           temp = 32; 
        }else{                      //everything else is a letter 
           temp = temp + 65; 
        }
       
        eBuffer[i] = temp;          //now add to the encrypted buffer   
        temp = '\0';                //reset temp
        /*if(i<100){
        printf("eBuffer[%d] is: %c \n", i, eBuffer[i]);
        }*/
        i++;
   }

   //now the eBuffer is ready to send back.  We continually send 1000 eBuffer unless the flag is set
    charWrite = send(clientFD, eBuffer, strlen(eBuffer), 0);//sending the cipher text back 
    if(charWrite <0) error("ERROR writing to socket",1);                
    memset(eBuffer, '\0', MAXLINE);//reset variables 
    memset(kBuffer, '\0', MAXLINE);
    memset(pBuffer, '\0', MAXLINE);
    if(flag){                       //if we encountered the newline earlier (flag set)
        stopLoop = 1;               //then don't loop again
    }

    }while(!stopLoop);
    
    //woohoo! all done!
}
