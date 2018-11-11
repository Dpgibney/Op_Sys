#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#define MAX_INPUT_SIZE 200
int main(int argc,char *argv[]){
char* input_raw = (char*)malloc(MAX_INPUT_SIZE*sizeof(char));
char* input = (char*)malloc(MAX_INPUT_SIZE*sizeof(char));
FILE *fptr;
printf("%s\n",argv[1]);
printf("%d\n",argc);

//check that a file was passed as an argument
if(argc!=2){
   printf("Improper usage: must pass image file as argument\n");
   return -1;
}
//check that file could open
//currently using binary mode
fptr = fopen(argv[1],"rb+");
if(fptr==NULL){
   printf("Opening file failed\n");
   return -1;
}

//will grab newline as well be aware
fgets(input_raw,MAX_INPUT_SIZE,stdin);
sscanf(input_raw,"%s",input);

//main shell like loop to ask user what to do
while(strcmp(input,"exit")!=0){
printf("%s\n",input_raw);


//get new input from user
fgets(input_raw,MAX_INPUT_SIZE,stdin);
sscanf(input_raw,"%s",input);
}
//free memory here
free(input);
free(input_raw);

}
