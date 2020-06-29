/*Name: Sarah Turner
 *Date: 28 April  
 *CS 344 PROJECT 2
 *Description: Uses an array of room structs to create files for seven 
 *random rooms and connections between random rooms. One room of seven 
 *is room type start, one is room type end, and the rest are mid.
 *references: Block 2-2.4, Block 2-2.2, Fisher Yates shuffle algorithm
 *            https://www.tutorialspoint.com/c_standard_library/
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

//******list of global variables******//

//our 10  room names 
char *roomNames[10]={"abbatoir", "chapel", "study", "latrine", "asylum", "ossuary", "barracks", "dungeon", "catacomb", "library"};

//our 3 room types 
char *roomType[3]={"START_ROOM", "MID_ROOM", "END_ROOM"};

//our room structs 
struct Room{
    int count;              //how many connections
    char *type;             //roomType
    char *name;             //roomNames
    char *connections[6];    //we add these below 
};    

//array of rooms 
struct Room roomGame[7];     

/*****************************************************************
 * Name: void initialize()
 * Description: uses shuffle to determine which names are chosen. 
 * initializes array of rooms with room names, type and sets connection
 * count to 0.
 *****************************************************************/
void initialize(){
    int num[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    //Fisher Yates shuffle algorithm 
    //we basically shuffle our list of 10 and select the first 7 to get our random room names 
    int i;
    int temp;
    for(i=9; i>0; i--){
        int j = (rand() % (i+1));
        temp = num[i];
        num[i] = num[j];
        num[j] = temp; 
    } 

   //pick our random rooms and initialize the array of rooms  
    for(i=0; i<7; i++){
       roomGame[i].name = roomNames[(num[i])];
       //printf("%s\n", roomGame[i].name);
       roomGame[i].count = 0;
       if (i > 0 && i<6){//the first room and last room will be the start and end rooms respectively 
            roomGame[i].type = roomType[1];//the rest are mid rooms
            //printf("%s\n", roomGame[i].type);
            
       }     
    }
    roomGame[0].type = roomType[0]; //our start room
    roomGame[6].type = roomType[2]; //our end room 
}


/*****************************************************************
 * Name: int isGraphFull()
 * Description: returns 1 if any room has less than 3 connections,
 * and returns 0 if all rooms have at least 3 connectoins. Used 
 * to evaluate if we are done connecting rooms.
 *****************************************************************/
int isGraphFull(){
    //printf("in graph is full\n");
    int i; 
    for (i=0; i<7;i++){
        if(roomGame[i].count < 3){
           return 1;
    }}
    
    return 0;

}

/*****************************************************************
 * Name: int GetRandomRoom()
 * Description: returns random number between 0 and 6. Only
 * returns the number if room at that index has less than 6 
 * connections already. This will be used as an index in our room array.
 *****************************************************************/
int GetRandomRoom(){
    int flag = 0;
    int i;
    while(flag == 0){
        i = rand() % 7;
        if(roomGame[i].count<6){
            flag = 1; 
        }
    }
    //printf("Random number: %d\n", i);
    return i; 
}

/*****************************************************************
 * Name: int ConnectionAlreadyExists(int x, int y)
 * Description: returns 0 if a connection from x to y already exists,
 * and returns 1 if it doesn't exist. Evaluation is done by seeing if 
 * room name at index x matches a connection at index y 
 *****************************************************************/
int ConnectionAlreadyExists(int x, int y){
    int yCount = roomGame[y].count;
    int i;
    char *xRoom = roomGame[x].name;
    //printf("Xroom: %s\n", xRoom);
    for(i=0;i<yCount;i++){
      char *yRoom = roomGame[y].connections[i]; 
      //printf("Yroom: %s\n", yRoom);
        if((strcmp(xRoom,yRoom)) == 0){
            //printf("already exists\n");
            return 0;
        }
    }
      //printf("doesn't exist\n");
      return 1;
}

/*****************************************************************
 * Name: void ConnectRoom(int x, int y)
 * Description: add room x to y connection and room y to x connection.
 * Increments the count for both x and y.
 *****************************************************************/
void ConnectRoom(int x, int y){
    //printf("connecting %s and %s rooms\n",roomGame[x].name, roomGame[y].name); 
    int xCount = roomGame[x].count;
    int yCount = roomGame[y].count;
    roomGame[x].connections[xCount] = roomGame[y].name;
    roomGame[y].connections[yCount] = roomGame[x].name;
    xCount = xCount +1;
    yCount = yCount +1;
    roomGame[x].count = xCount;
    roomGame[y].count = yCount;
    //printf("Xcount: %d, Ycount: %d\n", xCount, yCount);
}

/*****************************************************************
 * Name: void AddRandomConnection()
 * Description: adds a random and valid two way connection between rooms
 *****************************************************************/
void AddRandomConnection(){
   //printf("adding a connection\n");
   int i, j;  

       i = GetRandomRoom();
    do{
        j = GetRandomRoom();
    }
    while(i == j || ConnectionAlreadyExists(i, j) == 0);

    ConnectRoom(i, j);
}

/*****************************************************************
 * Name: void makeFiles()
 * Description: makes a new directory and writes files to the new
 * directory. There are seven files each formed from a room in the
 * room array 
 * *****************************************************************/
void makeFiles(){
    int p = getpid();
    char *dir = "turnesar.rooms.";
    char dirName[25];
    snprintf(dirName, 25,"%s%d", dir, p);
    int result = mkdir(dirName, 0755);
    if(result == 0){
        char addFile[40];
        FILE *curFile;
        int i, j;
        for(i=0;i<7;i++){
        char tmp[10];
        char t[1] = "/";
        snprintf(tmp, 11, "%s%s", t, roomGame[i].name);
        snprintf(addFile, 40,"%s%s",dirName, tmp);
        curFile = fopen(addFile, "w+");
        fprintf(curFile, "%s %s %s\n", "ROOM", "NAME:", roomGame[i].name);
            for(j=0; j<roomGame[i].count;j++){
            fprintf(curFile, "%s %d%s %s\n", "CONNECTION", j+1,":", roomGame[i].connections[j]);
    }
        fprintf(curFile, "%s %s %s\n", "ROOM", "TYPE:", roomGame[i].type);  
        fclose(curFile);
        }
    }
}

/*****************************************************************
 * Name: int main()
 * Description: seeds rand, initializes global variables, fills
 * the global variables, publishes them, exits program
 * *****************************************************************/
int main(){
    srand(time(NULL));  //seeding for the rand() uses 
    //printf("Initializing\n"); 
    initialize();
    //printf("Making Connections\n");
    while (isGraphFull() == 1){
        AddRandomConnection();
    }
    //printf("Writing the files\n");
    makeFiles();
    //printf("exiting\n");
    return 0;
}    
