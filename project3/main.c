#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#define MAX_INPUT_SIZE 200
int main(){
char* input_raw = (char*)malloc(MAX_INPUT_SIZE*sizeof(char));
char* input = (char*)malloc(MAX_INPUT_SIZE*sizeof(char));

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
}
