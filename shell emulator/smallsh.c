/* Name: Sarah Turner
 * Date: May 18 
 * CS 344 PROJECT 3
 * Description: This programs runs a shell with three
 * built in commands that runs foreground and background
 * processes. The built in commands are 'exit', 'status',
 * and 'cd'.  Exit kills all child processes and 
 * exits the program.  Status shows the exit/signal
 * that ended the last foregroundchild process. cd changes
 * the directory to the path specified, and with zero 
 * arguments defaults to the home directory. Lines
 * starting with # or blank lines are ignored.
 *
 * All user input lines are first evaluated for built in
 * commands and then parsed to input, output,
 * arguments and background flag if not a built in.
 * All $$ are expanded to pid.  
 *
 * Parsed line is then sent to be forked with input
 * and output set by user.  If background goes to 
 * /dev/null if not specified for input and output.
 * Then exec based on arguments in passed array.
 *
 * SIGINT handler in main ignores SIGINT. In the child
 * process, if not a background process, SIGINT is 
 * handled as default.  SIGTSTP handler in main runs
 * function that sets flag allowing or ignoring
 * background processes to be run. SIGTSTP is ignored in 
 * child process.
 *
 * References: 
 * All lectures in block
 * http://www.cplusplus.com/reference/cstring/strtok/
 * http://man7.org/linux/man-pages/man2/sigaction.2.html
 * https://www.geeksforgeeks.org/strstr-in-ccpp/
 * https://www.gnu.org/software/libc/manual/html_node/Process-Completion.html
 * *************************************************/

//library files 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

//constant sizes for strings and char arrays
#define MAXLINE 2048
#define MAXARG 512
#define MAXSTRING 256
#define MAXPID 50

/*************function declarations******************/
void catchSIGTSTP();                //signal action handler for SIGTSTP in parent    
int getInput();                     //gets user line and parses it to other functions
void timeToExit();                  //cleanup function 
void changeDirectory(char* newPath);//sets the global variable 
void printStatus(int exitStatus, int setExit, int setSignal);  //prints exit/signal for foreground
void forkMe(char** args, char* inF, char* outF, int b);        //does fork and exec, waitpid
void pidExtension(char* stringIn);                             //extends $$ to parent pid                             
/*******************end of function declarations******/

/*************global variables*************/
int goBackground = 1;               //flag for background processes 
char theDirectory[MAXLINE] = ".";   //holds current directory location  
pid_t allPid[MAXPID];               //holds all pids to kill them at exit
int bgPid[MAXPID] = {0};            //holds all our background pids
int fgPid = 0;                      //holds foreground pid 
int bgPidNumber = 0;                //number of pid    
/****************end of global vars***********/

/*---------------Now all the functions--------------------*/

/***********************************************
 * Name: int main()
 * Descritpion: Sets parent process sigaction handlers.
 * Runs loop that gets user input as long as exit
 * isn't selected. Upon exit, runs exit function and 
 * then exits.
 * Reference: Signals lecture
 * *****************************************/
int main(){
  int keepGoing = 1;
  
  struct sigaction SIGINTaction = {0};
  struct sigaction SIGTSTPaction = {0};
  
  //this is for CTRL-C, ignore it here 
  SIGINTaction.sa_handler = SIG_IGN;
  sigfillset(&SIGINTaction.sa_mask);
  SIGINTaction.sa_flags = 0;
  
  //this is for CTRL-Z
  SIGTSTPaction.sa_handler = catchSIGTSTP;
  sigfillset(&SIGTSTPaction.sa_mask);
  SIGTSTPaction.sa_flags = SA_RESTART;

  sigaction(SIGINT, &SIGINTaction, NULL);
  sigaction(SIGTSTP, &SIGTSTPaction, NULL);
 
  //while exit is not selected, getInput returns 0 if exit, returns 1 otherwise
  while(keepGoing){ 
    fflush(stdout);
    keepGoing = getInput();
   }
   timeToExit();   //kill child processes              
    return 0;
}  

/*********************************************
 * Name: void catchSIGTSTP(int signo)
 * Description: action handler for SIGTSTP
 * Prints message and sets global variable flag
 * to ignore & or not
 * references: signal and unix IO lecture
 * ******************************************/
void catchSIGTSTP(int signo){
   
   if(goBackground){
        char* message ="\nEntering foreground-only mode (& is now ignored)\n: "; 
        write(STDOUT_FILENO, message, strlen(message));
        goBackground = 0;
   }
   else{
        char* message = "\nExiting foreground-only mode\n: ";
        write(STDOUT_FILENO, message, strlen(message));
        goBackground = 1; 
  }      
}

/**************************************************************
 * Name: int getInput()
 * Description: gets input in a line from user. Parses into tokens
 * and evaluates them against the built in commands. Unique functions
 * for cd, status and exit. blanks and # are ignored.  After that
 * parses line for commands, input file, output file and trailing 
 * background function. Returns 1 if they don't exit, returns 0
 * if they do want to exit. Sends arguments to the forkMe function
 * reference: http://www.cplusplus.com/reference/cstring/strtok/
 *************************************************************/
 int getInput(){
    char inFile[MAXSTRING];
    char outFile[MAXSTRING];        
    char userLine[MAXLINE];         
    char *cmdList[MAXARG] = {NULL}; //holds args 
    char newPath [MAXSTRING];       //holds path for directory changes
    char *tempString = NULL;        //holds token
    int backProcess = 0;            //flag for trailing & 
    int i =0;                       //used to fill cmdList array 
    int builtIn = 0;                //flag used to parse as arguments or built in
    int inFilePresent = 0;          //know to send or not
    int outFilePresent =0;          

    memset(userLine, '\0', sizeof(userLine));
    memset(inFile, '\0', sizeof(inFile));
    memset(outFile, '\0', sizeof(outFile));

        printf(": ");
        fflush(stdout);
        if(fgets(userLine, MAXLINE,stdin)){
            if(strcmp(userLine, "\n")==0){
                builtIn = 1;                    //ignore a blank entry  
            }
            else{
                strtok(userLine, "\n");   
                tempString = strtok(userLine, " ");  //else lets read it
            }     
            if (tempString == NULL){
                builtIn = 1;
            }    
            if (tempString != NULL){                //ignore # comments
                if(strcmp(tempString,"#") == 0){
                    builtIn = 1; 
                }    
                else if(strcmp(tempString, "exit")==0){  //break the loop
                    return 0;
                }    
                else if(strcmp(tempString, "cd")==0){    //for change directory 
                    tempString = strtok(NULL, " ");
                  if(tempString != NULL){
                       pidExtension(tempString);        //check for $$ expansion
                        strcpy(newPath, tempString);    //new path specified    
                  }
                  else{
                        strcpy(newPath, getenv("HOME"));  //no args, must be home
                  }
                    changeDirectory(newPath);
                    builtIn = 1; 
                }
                else if(strcmp(tempString, "status")==0){  //print last status
                    printStatus(0,0,0); 
                    builtIn = 1; 
                }
                else if(tempString[0]=='#'){           //ignore #comments
                    builtIn = 1;
                }
            }                                           //not a built in!
            while(tempString != NULL && !builtIn){     
                if(strcmp(tempString, "<")==0){         //is there a infile?
                  tempString = strtok(NULL, " ");
                  pidExtension(tempString);
                  strcpy(inFile, tempString);
                  inFilePresent =1;
                }
                else if (strcmp(tempString, ">")==0) {   //outfile
                  tempString = strtok(NULL, " ");
                  pidExtension(tempString);
                  strcpy(outFile, tempString);
                  outFilePresent = 1;
                }                                       //background?
                else if (strcmp(tempString, "&")==0 && (strtok(NULL, " " ) == NULL)){
                    if (goBackground){
                        backProcess = 1; 
                        if(!inFilePresent){
                            strcpy(inFile, "/dev/null");
                        }
                        if(!outFilePresent){
                            strcpy(outFile, "/dev/null");
                        }
                    }
                }
                else{
                    pidExtension(tempString);            //$$ variable expansion
                   // printf("this added: %s\n", tempString);
                   // fflush(stdout);
                    cmdList[i] = tempString;            //add to args
                    i++; 
                }
                tempString = strtok(NULL, " ");        //proceed to next token
            }
         }
         /*used for test 
         int b = 0; 
         while(cmdList[b]!= NULL){
            printf("command list contains: '%s' ", cmdList[b]);
            b++;
         }
         fflush(stdout);
         */
        //time to FORK!   
        if(!builtIn){                                       //send to fork
            forkMe(cmdList, inFile, outFile, backProcess);
        }
        return 1;                                           //default return
 }



/**************************************************
 * Name: void pidExtension(char* stringIn)
 * Description: receives the string pointer for each user 
 * string and modifies it if $$ is in the string. 
 * Expands $$ to parent pid_t cast as an int.
 *
 * Used reference for strtok in getInput()
 * *************************************************/
void pidExtension(char* stringIn){
    char temp[MAXSTRING];
    char inter[4]= "$$";
    char* t = NULL;
    char copy[MAXSTRING];
    int i = 0; 

    strcpy(copy, stringIn);
    t = strstr(copy, "$$");  //does it have $$?
    if(t){
        t = strtok(copy, inter);  //split it 
        if(t){
            snprintf(temp, MAXSTRING, "%s%d", t, (int)getpid()); //make it word+pid
            strcpy(stringIn, temp);//copy into original string
            t = strtok(NULL, " "); //to flush the other token
        }
        else{
            snprintf(temp, MAXSTRING, "%d", (int)getpid());  //this is $$ on its own
            strcpy(stringIn, temp);
        }    
    }
    else{
        //printf("no modification\n");
    

    }
}

/***********************************************
 * Name: void changeDirectory(char* newPath)
 * Description: receives the newPath and sets
 * it to the global variable so forkMe can access it too
 * ************************************************/
void changeDirectory(char *newPath){
    DIR* currentDir; 
    currentDir = opendir(newPath);
    if (currentDir){
        strcpy(theDirectory, newPath);
        //set newPath to the global path     
    }
    else{
        printf("%s: directory does not exist\n", newPath);
        fflush(stdout);
        //error message and don't change global path  
    }    
    closedir(currentDir);
}

/************************************************
 * Name: void timeToExit()
 * Description: is called as a cleanup
 * function before exiting. Kills all the 
 * children if needed. Wow, that gets dark fast. 
 * **********************************************/
void timeToExit(){
    if(fgPid != 0){
        kill(allPid[0], SIGKILL);
    }
    int j = 0;
    for(j=0; j<50; j++){
        if(bgPid[j] != 0){
            kill(allPid[j], SIGKILL);
        }
    } 
    
}

/*******************************************
 * Name: void printStatus(int exitStatus, int setExit, int setSignal)
 * Description: used to set the exit or signal status and also to print
 * each argument is treated like a bit. so you would send 000 to print
 * and 010 to set exit status to 0 and 11 0 1 to set signal exit to 11
 * **************************************/
void printStatus(int exitStatus, int setExit, int setSignal){
    static int status = 0;    //starting status is static
    static int signalExit = 0; //boolean flag 
    if(setExit){                //if setExit is 1, then set the exit status   
        status = exitStatus;
        signalExit = 0;
    }
    else if (setSignal){        //else setSignal is 1, then set signal status
        status = exitStatus;
        signalExit = 1; 
    }
    else if(!setExit && !setSignal){  //then print the values as needed.
        if(signalExit){
            printf("terminated by signal %d\n", status);
            fflush(stdout);
        }
        else{
            printf("exit value %d\n", status);
            fflush(stdout);
        }
    }    
}

/******************************************
 * Name: void forkMe(char** args, char* inF, char* outF, int b)
 * Description: recieves the inputs from the user.  Creates a fork,
 * then validates the input and output file. Sets up the child process
 * and then execs based on the args.  Exits with 1 if any errors in the 
 * child process.  Signal handlers for SIGTSTP (SIG_IGN) and
 * SIGINT (SIG_DFL- for foreground only) are set up in the child process.
 * In the parent process, there is a waitpid for the foreground process
 * and a separate waitpid with WNOHANG for the array of background processes.
 * References: all lectures from this block
 * **********************************************************/
//going to fork and exec here 
void forkMe(char** args, char* inF, char* outF, int b){
    
    int stdinFile;                  //used to open incoming file
    int stdoutFile;                 //used to open outgoing file
    int result;                     //checking for dup2 status on files 
    int babyPid = -5;               //used to hold fork pid
    int fgExitStatus = -5;          //used for foreground exit    
    int bgExitStatus = -5;          //used for background exit 
    int fgStatus = 0;               //used to store signal/exit # for foreground 
    int bgStatus = 0;               //used to store signal/exit # for background
    int i = 1;                      //used in arrays 

    chdir(theDirectory); //changing directory to global path 
    babyPid = fork();    //creating our fork process 

    switch(babyPid){
        case -1:{
            perror("Hull Breach!\n");        //oh snap its a bad day 
            exit(1);
            break;
        }
        case 0:{                                    //child process
            struct sigaction SIGTSTP_action = {0};  //SIG_IGN for SIGTSTP for child processes
            sigfillset(&SIGTSTP_action.sa_mask);
            SIGTSTP_action.sa_flags = 0;
            SIGTSTP_action.sa_handler = SIG_IGN;
            sigaction(SIGTSTP, &SIGTSTP_action, NULL);
            
            //if not in the background, default action for interrupt
            if(!b){
                struct sigaction SIGINT_action = {0};
                sigfillset(&SIGINT_action.sa_mask);
                SIGINT_action.sa_flags = 0;
                sigaction(SIGINT, &SIGINT_action, NULL);
                SIGINT_action.sa_handler = SIG_DFL;
            }
            //first we set all our files 
            if (strcmp(inF, "\0")!=0) {      //if there is a inFile
            stdinFile = open(inF, O_RDONLY); //open it 
            if(stdinFile == -1){             //error opening 
               printf("cannot open %s for input\n",inF); 
               fflush(stdout);
               if(!b){
                printStatus(1, 1,0);        //set the exit value 
               } 
               exit(1); 
            }
            else{                           //file opened 
                result = dup2(stdinFile, 0);
                if(result == -1){           //if bad result read error, otherwise it worked 
                    printf("cannot read from %s location\n", outF);
                    fflush(stdout);
                    if(!b){
                        printStatus(1, 1,0);
                    } 
                    exit(1); 
                }                        
            }}
            if(strcmp(outF,"\0")!=0){           //same logic for the outFile as the Infile 
            stdoutFile = open(outF, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if(stdoutFile == -1){
               printf("cannot open %s for output\n",outF); 
               fflush(stdout);
               if(!b){
                printStatus(1, 1,0);
               } 
               exit(1); 
            }
            else{
                result = dup2(stdoutFile, 1);
                if(result == -1){
                    printf("cannot write to %s location\n", outF);
                    fflush(stdout);
                     if(!b){
                        printStatus(1, 1,0);
                    } 
                    exit(1); 
                }                        
            }}

            //test case 
            //printf("arg is: %s \n", *args);
            //fflush(stdout);
            
            //then we exec 
            if(execvp(*args, args)<0){          //if command can't be executed display message and exit 1
                printf("command cannot be executed\n");
                fflush(stdout);
                printStatus(1, 1,0);
                exit(1); 
            }
            else{
                fflush(stdout);
                //execute 
            }      
            break;
       }         
        default:{
            //parent things 
            if(b && goBackground){                          //if background flag set and background is allowed
                printf("background pid is %d\n", babyPid);
                fflush(stdout);
                while(bgPid[i]> 0 && i < MAXPID){           //find an empty pid spot 
                    i++;
                }
                bgPid[i] = babyPid;                         //then set it 
                allPid[i] = babyPid;                        //tracking all the pids in allPid
                bgPidNumber++;                              //tracking our number of bg pids 
                //used for test, and to alert you've reached 50 background processes 
                if(bgPidNumber > MAXPID-1){
                    printf("max # of background processes\n");
                    fflush(stdout);
                }
            }
            else if(!b){                                     //wait for the foreground process 
                fgPid = babyPid; 
                allPid[0] = babyPid;
                babyPid = waitpid(babyPid, &fgExitStatus, 0);//wait on our foreground process to exit 
                if(WIFEXITED(fgExitStatus) != 0){           //did it exit?
                    fgStatus = WEXITSTATUS(fgExitStatus);
                    printStatus(fgStatus, 1,0);             //if so set the status 
                    fgPid = 0; 
                }
                if(WIFSIGNALED(fgExitStatus) !=0){          //did a signal end it?
                    fgStatus = WTERMSIG(fgExitStatus);
                    printStatus(fgStatus, 0, 1);            //set signal in printStatus
                    fgPid = 0;
                    printStatus(0,0,0);                     //then print it
                }
                allPid[0] = 0; 
            }    
        //background process check after each process      
       bgPid[i] = waitpid(-1, &bgExitStatus, WNOHANG);
       while(bgPid[i]>0){                                   //loop through and check in the parent 
                if(WIFEXITED(bgExitStatus)){                //for background processes ending and 
                    bgStatus = WEXITSTATUS(bgExitStatus);   //then print out the status.
                    printf("background pid %d is done: exit status %d\n", bgPid[i], bgStatus);
                    fflush(stdout);
                }
                else if(WIFSIGNALED(bgExitStatus)){
                    bgStatus = WTERMSIG(bgExitStatus);
                    printf("background pid %d terminated by signal %d\n", bgPid[i], bgStatus);
                    fflush(stdout);
                }
             bgPid[i] = waitpid(-1, &bgExitStatus, WNOHANG);
       }      
                bgPid[i]=0;                                   //cleanup on the background pid  
                allPid[i]=0;
                bgPidNumber--;
    }            
   }
}   

