#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <stdbool.h>
char** addToken(char** instr, char* tok, int numTokens);
void printTokens(char** instr, int numTokens);
char * addPath(char * instr, char ** path);
char** parsePath(char * path);
 
int main() {
        char token[256];		/* holds instruction token*/
        char ** bucket;			/* array that holds all instruction tokens*/
        char temp[256];			/* used to split instruction tokens containing special characters*/
        int pid;
        int w;
        int status;
   	struct timeval start, finish;
	
	gettimeofday(&start, NULL);

        char* path;
        char ** pathtokens = parsePath(path);

        while (1) {
                char * tmp = (char *)malloc(100*sizeof(char));
                printf("%s@%s::%s%s",getenv("USER"),getenv("HOSTNAME"),get_current_dir_name(),"-> ");

                int numI = 0;                /* number of tokens in an instruction*/

                do {                            /* loop reads character sequences separated by whitespace*/
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

                } while ('\n' != getchar());    /*until end of line is reached*/

                //printTokens(bucket, numI);
		if(strcmp(bucket[0], "echo") == 0){
			if(bucket[1] != NULL){
				char tempChar[1];
				char *tempS = bucket[1];
				char * ePath;
				memcpy(tempChar, bucket[1],1);
				if(tempChar[0] ==  '$'){
					strcpy(tempS,tempS+1);
					if(ePath = getenv(tempS)){
						printf("%s\n",ePath);
					}
					else{
						printf("The path entered is not an envionmental variable\n");
					}
				}
				else{
					printf(bucket[1]);
					printf("\n");
				}
			}
			else{
				printf("Nothing to check if it is an enviornmental variable\n");
			}
		}
		else if(strcmp(bucket[0], "exit")==0){
			break;
		}
		else if(strcmp(bucket[0], "cd")==0){
			if(bucket[1] != NULL){
				if(bucket[2] == NULL){
					if(chdir(bucket[1])!= -1){
						chdir(bucket[1]);
					}
					else{
						printf("Error target is not a directory\n");
					}
				}
				else{
					printf("Error more than one argument present\n");
				}
			}
		}
		else if(strcmp(bucket[0], "io")==0){
			FILE *fp;
                        
                        char file_name[50];

                        if ((pid = fork()) == 0){
                        bucket[1] = addPath(bucket[1],pathtokens);
                        char ** new_bucket = &bucket[1];
                        //printf("%s\n",bucket[0]);                        
			//execute program av[0] with arguments av[0]... replacing this program
                        printTokens(new_bucket, numI-1);
                        execv(new_bucket[0],new_bucket);
                        //execv(bucket[1],&bucket[1]);
                        printf("io comada");
                        /*fprintf(stderr, "can't execute %s\n", av[0]);*/
                        exit(EXIT_FAILURE);
                        }
                        sprintf(file_name,"/proc/%d/io",pid);
                        printf("%s\n",file_name);
                        
                        fp = fopen(file_name,"r");
                        if(fp!=NULL){
                        printf("%s\n",fp);
                        }
                        
                         while ((w = wait(&status)) != pid && w != -1){
                             continue;          
                        }         
                        //if(pid != 0){
                        printf("io comada");
		}

                else if ((pid = fork()) == 0)
                {
                        /* this is the forked child process that is a copy of the running program*/
                        /*if(newin == false){
                        dup2(tty, 0);  // force stdin from tty
                        }else{
                        dup2(fdin, 0);
                        close(fdin);
                        }
                        if(newout == false){
                        dup2(tty, 1); // force stdout to tty
                        }else{
                        dup2(fdout, 1);
                        close(fdout);
                        }
                        dup2(fdout, 2); // force stderr to tty
                        close(tty);
                        //last argument must be NULL for execvp()
                        av[ac] = NULL; */
               
                        bucket[0] = addPath(bucket[0],pathtokens);
                        printTokens(bucket, numI);
                        //printf("%s\n",bucket[0]);                        
			//execute program av[0] with arguments av[0]... replacing this program
                        execv(bucket[0],bucket);
                        /*fprintf(stderr, "can't execute %s\n", av[0]);*/
                        exit(EXIT_FAILURE);
                        }
                         while ((w = wait(&status)) != pid && w != -1){
                             continue;          
                        }         
                        //if(pid != 0){
                        //    int childstatus;
                        //    waitpid(pid,&childstatus,WNOHANG);
                        //} 

        }  /*until "exit" is read in*/
        free(bucket);	/*free dynamic memory*/
        printf("Exiting...\n");
	gettimeofday(&finish, NULL);
	printf("\tSession time: %ds\n",(finish.tv_sec-start.tv_sec));
        return 0;
}

/*reallocates instruction array to hold another token,
  returns new pointer to instruction array */
char** addToken(char** instr, char* tok, int numTokens)
{
        int i;
        
        char** new_arr;
        new_arr = (char**)malloc((numTokens+2) * sizeof(char*));				
        /*copy values into new array*/
        for (i = 0; i < numTokens; i++)
        {
                new_arr[i] = (char *)malloc((strlen(instr[i])+1) * sizeof(char));
	        strcpy(new_arr[i], instr[i]);
        }
        
        /*add new token*/
        new_arr[numTokens] = (char *)malloc((strlen(tok)+1) * sizeof(char));
        strcpy(new_arr[numTokens], tok);
        
        if (numTokens > 0)
        free(instr);
        new_arr[numTokens+1] == NULL;
        return new_arr;
}

void printTokens(char** instr, int numTokens)
{
        int i;
        printf("Tokens:\n");
        for (i = 0; i < numTokens; i++){
                printf("#%s#\n", instr[i]);
        }
}

char * addPath(char * instr, char ** path){
       int i = 0;
       int tmp = 0;
       struct dirent *dp;
       DIR *d;
       char * tmp_command;
       char * tmp_instr = (char*)malloc(100*sizeof(char));
       bool containsslash = false;
        while(instr[i]!='\0'){
               if(instr[i] == '/'){
                   containsslash = true;
               }
               i++;
           } 
       i = 0; 
       if(instr[0]=='/' || containsslash){
           while(instr[i]!='\0'){
               if(instr[i] == '/'){
                  tmp = i;
               }
               i++;
           }
           i = 0;
           strcpy(tmp_instr,instr);
           tmp_command = &instr[tmp+1];
           tmp_instr[tmp] = '\0';
           d = opendir(tmp_instr);
           if(d == NULL || strcmp(tmp_command,"")==0){
               printf("%s: Command not found.\n",instr);
               return NULL;
           }else{
		   while((dp = readdir(d))!= NULL){
			   if(strcmp(tmp_command,dp->d_name)==0){
				   return instr;
			   }
		   }
           }
               printf("%s: Command not found.\n",instr);
               return NULL;
       }else if(containsslash){
           while(instr[i]!='\0'){
               printf("relative\n");
               if(instr[i]=='/'){
                   printf("relative or absolute\n");
               }
               i++;

           }
           i = 0;
       }
       //add absolute path for execution
       while(path[i] != NULL){
            //printf("%s\n",path[i+1]);
            d = opendir(path[i]);
            if(d){
                while((dp = readdir(d))!= NULL){
                    //printf("%s\n", dp->d_name);
                    if(strcmp(instr,dp->d_name)==0){
                       char * new_path = (char *)malloc(200*sizeof(char));
                       strcpy(new_path,path[i]);
                       strcat(new_path,"/");
                       strcat(new_path,dp->d_name);
                       return new_path;
                    }
                }
            }
            i++;
            closedir(d);
       }
       printf("%s: Command not found.\n",instr);
       return; 

}
 
char** parsePath(char* path){
        path = getenv("PATH");
        //prints path for testing
        //        //printf("%s\n",path);
        int i = 0;
        //number of different path directories to check
        int paths = 0;
        char ** pathtokens = malloc(100*sizeof(char *));
        pathtokens[paths] = path;
        while(path[i]!='\0'){
             //printf("%c\n",path[i]);
             if(path[i]==':'){
                  path[i]='\0';
                  paths++;
                  pathtokens[paths] = &path[i+1];
                  }
                  i++;
             }
        //prints output for testing
        //printf("%d\n",paths);
        //for(i = 0; i < paths; i++){
        //    printf("%s\n",pathtokens[i]);
        //}
        //add a null so that my add path function knows where to end
        pathtokens[paths+1] = NULL;
        return pathtokens;
}
