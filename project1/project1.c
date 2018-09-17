#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
char** addToken(char** instr, char* tok, int numTokens);
void printTokens(char** instr, int numTokens);
 
int main() {
        char token[256];      	// holds instruction token
        char ** bucket;			// array that holds all instruction tokens
        char temp[256];			// used to split instruction tokens containing special characters
        int pid;

        while (1) {
                printf("Please enter an instruction:");

                int numI = 0;                // number of tokens in an instruction

                do {                            // loop reads character sequences separated by whitespace
                        scanf( "%s", token);

                        int i;
                        int start;

                        start = 0;

                        for (i = 0; i < strlen(token); i++)
                        {
                                if (token[i] == '|' || token[i] == '>' || token[i] == '<' || token[i] == '&') 
                                {
                                        if (i-start > 0)
                                        {
                                                memcpy(temp, token + start, i - start);
                                                temp[i-start] = '\0';
                                                bucket = addToken(bucket, temp, numI);
                                                numI++;

                                        }

                                        char specialChar[2];
                                        specialChar[0] = token[i];
                                        specialChar[1] = '\0';

                                        bucket = addToken(bucket,specialChar,numI);
                                        numI++;

                                        start = i + 1;
                                }
                        }
                        if (start < strlen(token))
                        {
                                memcpy(temp, token + start, strlen(token) - start);
                                temp[i-start] = '\0';
                                bucket = addToken(bucket, temp, numI);
                                numI++;

                        }

                } while ('\n' != getchar());    //until end of line is reached

                printTokens(bucket, numI);
                if ((pid = fork()) == 0)
                {
                        // this is the forked child process that is a copy of the running program
                       // if(newin == false){
                       // dup2(tty, 0); // force stdin from tty
                       // }else{
                       // dup2(fdin, 0);
                       // close(fdin);
                       // }
                       // if(newout == false){
                       // dup2(tty, 1); // force stdout to tty
                       // }else{
                       // dup2(fdout, 1);
                       // close(fdout);
                       // }
                       // dup2(fdout, 2); // force stderr to tty
                       // close(tty);
                       // // last argument must be NULL for execvp()
                       // av[ac] = NULL;
                        // execute program av[0] with arguments av[0]... replacing this program
                        execv(bucket[0],(char *[]){bucket[0],NULL });
                        if(pid==0){
                            printf("whoops\n");
                        }
                        //fprintf(stderr, "can't execute %s\n", av[0]);
                        exit(EXIT_FAILURE);
                        } 

        }  //until "exit" is read in
        free(bucket);	//free dynamic memory
        printf("Exiting...\n");

        return 0;
}

//reallocates instruction array to hold another token,
////returns new pointer to instruction array
char** addToken(char** instr, char* tok, int numTokens)
{
        int i;
        
        char** new_arr;
        new_arr = (char**)malloc((numTokens+1) * sizeof(char*));				
        //copy values into new array
        for (i = 0; i < numTokens; i++)
        {
                new_arr[i] = (char *)malloc((strlen(instr[i])+1) * sizeof(char));
	        strcpy(new_arr[i], instr[i]);
        }
        
        //add new token
        new_arr[numTokens] = (char *)malloc((strlen(tok)+1) * sizeof(char));
        strcpy(new_arr[numTokens], tok);
        
        if (numTokens > 0)
        free(instr);

        return new_arr;
}

void printTokens(char** instr, int numTokens)
{
        int i;
        printf("Tokens:\n");
        for (i = 0; i < numTokens; i++)
                printf("#%s#\n", instr[i]);
}
