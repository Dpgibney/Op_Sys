#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
int main(){
        FILE *fptr;
	char* str = (char*)malloc(100*sizeof(char));
	fptr = fopen("example.txt","w+r+");
	fprintf(fptr,"test line 1 \n");
	fprintf(fptr,"test line 2 \n");
	fprintf(fptr,"test line 3 \n");
	rewind(fptr);
	fscanf(fptr,"%s",str);
	printf("%s\n",str);
	fclose(fptr);
}
