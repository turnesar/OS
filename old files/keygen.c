//syntax is keygen keyLength
/***********************************
 *Name: Sarah Turner
 *Date: 28 May 2019, CS344 PROJECT 4
 *Description: 
 *
 * ***********************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]){

    srand(time(0));
    char randos[28] = "  ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    //must be 28 for the modulus  
    //long userInput = strtol(argv[1], NULL, 10);
    int i = 0;
    int num; 
    
    //error messages first
    //error for empty input
    //error for invalid characters 
    //use fprintf(stderr, "blah\n");
    if(argv[1]){ 
    for(i=0; i< atoi(argv[1]); i++){
        num = rand() % 27;
        printf("%c", randos[num]);
    }

    printf("\n");
    }
return 0; 
}
