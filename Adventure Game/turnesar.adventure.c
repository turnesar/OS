/* Name: Sarah Turner
 * Date: April 28
 * CS 344 PROJECT 2
 * Description: First searches within the current directory for newest fileName (subdirectory in this case) with defined
 * prefix.  Once directory has been found, then goes into that directory and copies the file names into global variable 
 * names.  Directory name is passed to another function. There all files in the passed directory are parsed and data pulled
 * from them to initialize global variables.  Finally, the game is started with all gameplay done by accessing global 
 * variable values.  Two loops exist in the game play, an outer loop that ends when the END_ROOM is reached and an 
 * inner loop that receives user inputs and validates them. The inner loop is exited only when valid input is received.
 * Invalid data gives an error, and then resets the loop.  Upon reaching the END_ROOM the outer loop terminates 
 * and a congratulatory message is displayed giving number of steps and path taken.  
 *
 * Also returns current time of day by utilizing a second thread and mutex. If the player types 'time' at prompt a second thread 
 * writes current time in format '1:30pm, Tuesday, September 13, 2016' to a file currentTime.txt.
 * The main thread then will read this time value and print it. The file is in the directory after exit 
 * *********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

//function declaration
void buildGame(char* name);

/*************global variables*************/
//our struct for each Room
struct Room{
    int count; 
    char type[15];
    char name[15]; 
    char connections[6][15];
};

//our array of rooms 
struct Room roomGame[7];

//the mutex *dum dum daaaa* 
pthread_mutex_t theMutex = PTHREAD_MUTEX_INITIALIZER;

/****************end of global vars***********/

 
/**********************************************
 *Name: void* timeHack()
 *Description: Our special second thread function
 *that loads the timestamp into a time struct tm*
 *where a formatted time stamp is copied into a 
 *currentTime.txt file 
 *
 *ref: http://man7.org/linux/man-pages/man3/strftime.3.html
 * **********************************/
void* timeHack(){
    
    time_t tickTock;                //current time 
    struct tm* timeStruct;          //holds all the time values 
    char timeString[100];           //holds our formatted string
    time(&tickTock);                
    timeStruct = localtime(&tickTock); //initialize with current time 
                                       //create formatted string 
    strftime(timeString, 100, "%l:%M%P, %A, %B %d, %G", timeStruct); 
    //printf("timeString: %s\n", timeString);

    FILE *timeFile;                    //write to file 
    timeFile = fopen("currentTime.txt", "w+");
    fprintf(timeFile, "%s", timeString);
    fclose(timeFile);
 }
/********************************
 *Name: void* releaseTheMutex()
 *Description: returns 0 if thread was created,returns 1 if it didn't
 *new thread calls timehack.
 *
 *-->honestly found this pretty confusing, I'm still reading a lot
 * on this as it is blowing my mind. I didn't feel comfortable 
 * weaving it throughout my main as suggested, but it seems to be 
 * working because I'm not getting my sadface message...anymore
 *
 *ref: concurrency lecture 
 *ref: https://docs.oracle.com/cd/E19455-01/806-5257/sync-12/index.html
 * *********************************/
void* releaseTheMutex(){
    pthread_t timeThread;               //holder for function thread 
    pthread_mutex_lock(&theMutex);      //lock it down 

    if(pthread_create(&timeThread,NULL,timeHack,NULL)!= 0){
            printf("Mutex not released :(");
            return (void *)1;
    }

    pthread_mutex_unlock(&theMutex);    //yay it worked, unlock!
    pthread_join(timeThread, NULL);     //per the hints 
    return 0; 
}

/***************************
 *Name: void printMagic()
 *Description: opens currentTime.txt and prints content.
 *Then closes file.
 * ************************/
void printMagic(){
    FILE *timeFile;
    char buff[100];
    memset(buff, '\0', sizeof(buff));
    
    timeFile = fopen("currentTime.txt","r");
    if(timeFile != NULL){                             //if the file opens 

            while(fgets(buff, sizeof(buff), timeFile)){                       //pulling lines from file 
               printf("\n%s\n", buff);
               memset(buff, '\0', sizeof(buff));
            }
    }
    fclose(timeFile);
}    


/**********************************************************************************
 *Name: void initialize()
 *Description: finds the most recent directory by performing a stat() function call on rooms
 *directories in the same directory as the game and passes the directory
 * with the most recent st_mtime component of the returned stat struct
 *references: section 2.4
 * ********************************************************************/
void initialize(){
    char myPreset[25] = "turnesar.rooms.";      //searching for items starting with this
    char newestDir[256];                        //saving the name of the target dir here 
    memset(newestDir, '\0', sizeof(newestDir)); //initialize 
    DIR* currentDir;                            //used to navigate directories   
    struct dirent *fileDir;                     //struct used to access name of our subdirectory
    struct stat dirTrait;                       //stats for the st_mtime 
    int newestTime = -1;                        //compare to see newest directory 
    int i =0;

    currentDir = opendir(".");                  //open current directory    
    if(currentDir){
        while((fileDir = readdir(currentDir)) != NULL){

            if(strstr(fileDir->d_name, myPreset)!=NULL){   //if subdir starts with our prefix
                //printf("found %s\n", fileDir->d_name);
                stat(fileDir->d_name, &dirTrait);

                if((int)dirTrait.st_mtime > newestTime){  //if subdir is newer then..
                    newestTime = (int)dirTrait.st_mtime;
                    memset(newestDir, '\0', sizeof(newestDir));
                    strcpy(newestDir, fileDir->d_name);   //copy it into our newestDir string
                }
            }
        }
     }
    closedir(currentDir);   

    //printf("most current directory is: %s\n", newestDir);

    currentDir = opendir(newestDir);                    //open the newdir and copy the file names into the struct of rooms 
    int t =0;
    if(currentDir){
        while((fileDir = readdir(currentDir)) != NULL){
            if (t >1){                                       //we don't want the . and the .. -->so we skip those
                strcpy(roomGame[i].name, fileDir->d_name);   //copy each file in the directory into our name field in room struct array
                //printf("file list: %s\n", roomGame[i].name);
                i = i+1; 
            }
        t = t+1; 
        }
    }

    closedir(currentDir);

    buildGame(newestDir);                               //pass the directory name to build our room array 
}
/*****************************************************
 * *void buildGame(char* name)
 *Receives directory name. Opens directory and then copies each
 *file's data into the array of structs. 
 * ****************************************************/
void buildGame(char* dirname){
    //printf("in buildgame\n");
    char buff[50];              //holds each line of file
    FILE *inFile;
    int firstLine = 0;          //used to skip over first line of file, we already have that one  
    int i = 0;                  //increment in array of structs
    int j = 0;                  //increment connections for one room 
    char fileName[50];          //fileName that includes dir/name
    char t[1] = "/";            //used in filepath 
    char tmp[25];               //used to help connect our /filename to dirname 
                                //initialize our char arrays 
    memset(tmp, '\0', sizeof(tmp));
    memset(buff, '\0', sizeof(buff));
    memset(fileName, '\0', sizeof(fileName));
    //loops throught each room in room array and sets the variables 
    while(i<7){
            snprintf(tmp, 25, "%s%s", t, roomGame[i].name);                         //make the filepath 
            snprintf(fileName, 50, "%s%s", dirname, tmp);                           //--> directory/file
            //printf("fileName %d: %s \n",i, fileName);
            memset(tmp, '\0', sizeof(tmp));                                         //clear out for next time 
            inFile = fopen(fileName, "r");
            if(inFile != NULL){  

                    while(fgets(buff, sizeof(buff), inFile)){                       //pulling lines from file 
                    //printf("buffer: %s\n", buff); 

                        if(firstLine == 1){                                         //if past the first line 
                         if(buff[0] == 'C'){                                        //connections or room type 
                                sscanf(buff, "%*s %*d%*s %s", roomGame[i].connections[j]);
                                //printf("file %d c: %s \n",i, roomGame[i].connections[j]);
                                j = j+1;                                            //incremement connections list 
                            }
                            if(buff[0] == 'R'){
                                sscanf(buff, "%*s %*s %s", roomGame[i].type);
                                //printf("file %d t: %s \n", i, roomGame[i].type);
                            } 
                        }    
                        if(firstLine == 0){                                         //skip the first line 
                            firstLine =1;
                            //printf("file %d n: %s \n",i, roomGame[i].name);
                        }    
                        memset(buff, '\0', sizeof(buff));
                }
            roomGame[i].count = j;                                                  //set our count variable last 
           // printf("file %d ct: %d\n",i,roomGame[i].count);
            i = i +1;                                                               //increment our room array index 
            j = 0;                                                                  //reset our connections index for next time
            firstLine = 0;                                                          //reset firstLine flag 
            fclose(inFile);
            memset(fileName, '\0', sizeof(fileName));
        }}

}    

/***************************************************
 * Name: void playGame()
 * Description: sets index to START_ROOM and cycles through 
 * map using array of structs based on user inputs.  Validates
 * user inputs to ensure they match connections listed for 
 * current index. Increments count of valid moves and adds
 * visited room list to path array. Upon reaching the 
 * END_ROOM the loops terminate and the final summary is displayed
 *
 * If user enters 'time' as input, checks to make sure thread was 
 * created. If so, calls function to print time file. 
 * ***********************************/
void playGame(){
    //printf("in playGame\n");
    printf("\n");           //puts a space between command and gameplay 
    int i = 0;              //used for looping 
    char userInput[15];     //user input from stdin
    int flag = 0;           //used to exit upon END_ROOM
    int idx = 8;            //holds idx of current room 
    int good = 0;           //valid flag for userInput 
    char *path[100];        //holds the path to victory
    int gameCount = 0;      //holds the room count
    int xCount = 0;         //holds # of connections 
    int releaseIt =0;       //release the mutex?
    memset(userInput, '\0', sizeof(userInput));
   
    //first we find our start room, and set the initial index 
    while(idx == 8){
        //printf("setting start: %s\n", roomGame[i].type);
        if((strcmp(roomGame[i].type, "START_ROOM")) ==0){
            idx = i;
        } 
     i = i+1;    
    }

    while(flag == 0){       //outer loop cares about reaching END_ROOM, flag is 0 means not there yet 
        
        do{                 //this do-while loop cares about the next valid input, will repeat unless it receives good input 
            printf("CURRENT LOCATION: %s\n", roomGame[idx].name);
            printf("POSSIBLE CONNECTIONS:");
            xCount = roomGame[idx].count;
                            //print connections with commas in between, unless at the end use a period 
            for(i=0;i<xCount;i++){
                if(i<xCount-1){
                printf(" %s,", roomGame[idx].connections[i]);
                }
                else{
                printf(" %s.", roomGame[idx].connections[i]);
                }
            }

                do{
                    printf("\nWHERE TO? >");        
                    fgets(userInput, 15, stdin);
                    strtok(userInput, "\n");                    //remove the newline 
                    if((strcmp(userInput, "time")) == 0){       //thread check
                            //are we going to thread?
                        if(releaseTheMutex()==0){               //open the second thread
                            printMagic();
                        }                                       //and now print in our main thread 
                        memset(userInput, '\0', sizeof(userInput));
                    }
                    else{
                        releaseIt =1;                           //no threading 
                    }
                }while (releaseIt==0);                          //ask where to again if we threaded 

                    //make sure their input is in the connections list, if it is good, good flag is 1
                for(i=0;i<xCount;i++){
                if((strcmp(userInput, roomGame[idx].connections[i]))==0) {
                    good = 1; 
            }}
            if(good == 0){              //this means userInput was not in the connections list 
               printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN\n\n");
               memset(userInput, '\0', sizeof(userInput));
            }
            
        }while (good == 0); //keep looping at same index until good = 1, aka match userInput to connection

        releaseIt = 0;   
        good = 0;           //reset our good flag, releaseIt flag and inc gameCount and path list
                            //add to our path, but don't include the start room
        if(gameCount > 0){
            path[gameCount-1] = roomGame[idx].name; 
        }
        gameCount = gameCount + 1;  //inc game count 

                            //find our next index based on user choice 
        for(i=0;i<7;i++){
            if((strcmp(userInput, roomGame[i].name)) == 0){
                idx = i; 
        }}
                             //check to see if we need to end the outer loop, aka in the end room  
        memset(userInput, '\0', sizeof(userInput));
        printf("\n\n");                
                            //if we're in the end room, add to path list and then exit the loop 
        if ((strcmp("END_ROOM", roomGame[idx].type)) == 0){     
            path[gameCount-1] = roomGame[idx].name; 
            flag = 1;
        }
    }

   //its the end, print the deets 
    printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
    printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", gameCount);
    for(i=0;i<gameCount; i++){
        printf("%s\n", path[i]);
    }    

}

/************************
 *Name: int main()
 *Description: call to initialize variables, then gamePlay, exits  
 * *********************/
int main(){
  initialize();     //initialize our global variables, calls a nested function 
  playGame();       //gameplay done here 
  return 0;
}  
