#include <stdio.h> 
#include <dirent.h> 
#include <string.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <stdbool.h>
#include <errno.h>
struct queue{
	int position;
	int pid;
	bool state;
	char *cmd;
};

char** addToken(char** instr, char* tok, int numTokens);
void printTokens(char** instr, int numTokens);
char * addPath(char * instr, char ** path);
char** parsePath(char * path);
void redirect(char *bucket[], char ** pathtoken);
struct queue* allocate(struct queue *processes, int num);
bool ifBackground(char** instr, int num);
bool containsspecialchar(char ** bucket);
char * expandenv(char* env);
void stillrunning(struct queue* processes, int processcount);
bool isredirectchar(char ** bucket);
bool shouldpipe(char ** bucket);
void dopipe(char ** bucket,char ** pathtokens);

int main() {
	char token[256];		/* holds instruction token*/
	char ** bucket;			/* array that holds all instruction tokens*/
	char temp[256];			/* used to split instruction tokens containing special characters*/
	char *bucket_pop[256];
	int pid;
	int w;
	int status;
	struct timeval start, finish;
	struct queue *processes;
	char* path;
	char ** pathtokens;
	pathtokens = parsePath(path);
	int processcount = 0;

	gettimeofday(&start, NULL);

	while (1) {
		stillrunning(processes,processcount);
		char * tmp = (char *)malloc(100*sizeof(char));
		memset(tmp, '\0', 100*sizeof(char));
		printf("%s@%s::%s -> ",getenv("USER"),getenv("HOSTNAME"),get_current_dir_name());

		int numI;                /* number of tokens in an instruction*/
		numI = 0;

		do {                            /* loop reads character sequences separated by whitespace*/
			//printf("path: %s %s %s \n",pathtokens[0],pathtokens[1],pathtokens[2]);
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


		bucket[numI] = NULL;

		if((ifBackground(bucket, numI))&& (numI > 1)){
			if(strcmp(bucket[0],"&")==0){ //&cmd
				bucket = &bucket[1];
				numI = numI - 1;
			}
		}
		if(ifBackground(bucket, numI)){
			if((bucket[numI-1]!=NULL) && (strcmp(bucket[numI-1],"&")==0) && (strcmp(bucket[0],"&")!=0) && !containsspecialchar(bucket)){ //CMD &
				if(strcmp(bucket[0],"&")==0){
					printf("Error there is no command");
				}
				else{
					if((pid = fork())==0){
						bucket[0] = addPath(bucket[0],pathtokens);
						bucket[numI-1] = NULL;
						execv(bucket[0],&bucket[0]);
						printf("Error there is no command");
						exit(EXIT_FAILURE);
					}
					printf("pid %d\n", pid);
					processes = allocate(processes, processcount);
					processcount++;
					processes[processcount-1].pid = pid;
					processes[processcount-1].position = 1;
					processes[processcount-1].state = true;
					processes[processcount-1].cmd = (char *)malloc(256*sizeof(char));
					strcpy(processes[processcount-1].cmd, bucket[0]);
					bucket[numI-1] = NULL;
					int i = 1;
					while(bucket[i] != NULL){
						strcat(processes[processcount-1].cmd, " ");
						strcat(processes[processcount-1].cmd, bucket[i]);
						i++;
					}
					printf("process: %s\n",processes[processcount-1].cmd);
				}
			}
			else if((bucket[0]!=NULL) && (strcmp(bucket[0],"&")==0)){
				if((bucket[numI-1]!=NULL) && (strcmp(bucket[numI-1],"&")==0)){ //&cmd&
					if((strcmp(bucket[1],"&")==0)||strcmp(bucket[1],"|")==0){
						printf("Error there is no command");
					}
					//behaves like CMD&
					else{
						if((pid = fork()) == 0){
							bucket[1] = addPath(bucket[1],pathtokens);
							bucket[numI-1] = NULL;
							execv(bucket[1],&bucket[1]);
							printf("Error there is no command");
							exit(EXIT_FAILURE);
						}
						printf("pid %d\n",pid);
						processes = allocate(processes,processcount);
						processcount++;
						processes[processcount-1].pid = pid;
						processes[processcount-1].position = 1;
						processes[processcount-1].state = true;
						processes[processcount-1].cmd = (char *)malloc(256*sizeof(char));
						strcpy(processes[processcount-1].cmd,bucket[1]);
						int i = 2;
						while(bucket[i] != NULL){
							strcat(processes[processcount-1].cmd," ");
							strcat(processes[processcount-1].cmd,bucket[i]);
							i++;
						}

					}


				}
			}
			else if((bucket[numI-1] != NULL)&&(strcmp(bucket[numI-1],"&")==0)){
				if(strcmp(bucket[numI-3],">")==0){ //cmd > file&
					if(strcmp(bucket[0],">")==0){
						printf("Error there is no command\n");
					}
					else{
						bucket[numI-3] = NULL;
						if((pid = fork())==0){
							int outfile;
							if((outfile = open(bucket[numI-2],  O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH))==-1){
								fprintf(stderr, "shell: error creating file: %s\n", strerror(errno));
								return(EXIT_FAILURE);
							}
							dup2(outfile, 1);
							close(outfile);
							bucket[0] = addPath(bucket[0],pathtokens);
							execv(bucket[0],bucket);
							fprintf(stderr, "couldnt execute %s\n", strerror(errno));
							exit(EXIT_FAILURE);
						}
						printf("pid %d\n", pid);
						processes = allocate(processes,processcount);
						processcount++;
						processes[processcount-1].pid = pid; processes[processcount-1].position = 1;
						processes[processcount-1].state = true;
						processes[processcount-1].cmd = (char *)malloc(256*sizeof(char));
						strcpy(processes[processcount-1].cmd, bucket[0]);
						int i = 1;
						while(bucket[i] != NULL){
							strcat(processes[processcount-1].cmd," ");
							strcat(processes[processcount-1].cmd, bucket[i]);
							i++;
						}
					}

				}
				else if(strcmp(bucket[numI-3],"<")==0){ //cmd < file&
					bucket[numI-3] = NULL;
					if((pid = fork())==0){
						int infile;
						if((infile = open(bucket[numI-2],  O_RDONLY , S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH))==-1){
							fprintf(stderr, "shell: error creating file: %s\n", strerror(errno));
							return(EXIT_FAILURE);
						}
						dup2(infile, STDIN_FILENO);
						close(infile);
						bucket[0] = addPath(bucket[0],pathtokens);
						execv(bucket[0],bucket);
						fprintf(stderr, "couldnt execute %s\n", strerror(errno));
						exit(EXIT_FAILURE);
					}
					printf("pid %d\n", pid);
					processes = allocate(processes,processcount);
					processcount++;
					processes[processcount-1].pid = pid;
					processes[processcount-1].position = 1;
					processes[processcount-1].state = true;
					processes[processcount-1].cmd = (char *)malloc(256*sizeof(char));
					strcpy(processes[processcount-1].cmd, bucket[0]);
					int i = 1;
					while(bucket[i] != NULL){
						strcat(processes[processcount-1].cmd," ");
						strcat(processes[processcount-1].cmd, bucket[i]);
						i++;
					}
				}
				else{ // cmd1 | cmd2 &
					//hardest implementation
					int tmpfile[2];
					bucket[numI-1] = NULL;
					int ispipe = 0;
					int x;
					for(x = 0; x < numI-1; x++){
						if(strcmp(bucket[x], "|")==0){
							ispipe = x;
						}
					}
					if(strcmp(bucket[ispipe-1],"&")==0){
						printf("Error: not a vaild input\n");
					}
					else{
						bucket[ispipe] = NULL;
						pipe(tmpfile);
						if((pid = fork())==0){
							int infile;
							if((infile = open(bucket[numI-2],  O_RDONLY , S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH))==-1){
								fprintf(stderr, "shell: error creating file: %s\n", strerror(errno));
								return(EXIT_FAILURE);
							}
							dup2(infile, STDIN_FILENO);
							close(infile);
							bucket[0] = addPath(bucket[0],pathtokens);
							execv(bucket[0],bucket);
							fprintf(stderr, "couldnt execute %s\n", strerror(errno));
							exit(EXIT_FAILURE);

						}
						if((pid = fork())==0){
							int outfile = tmpfile[2];
							if((outfile = open(bucket[numI-2],  O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH))==-1){
								fprintf(stderr, "shell: error creating file: %s\n", strerror(errno));
								return(EXIT_FAILURE);
							}
							dup2(outfile, 1);
							close(outfile);
							bucket[0] = addPath(bucket[ispipe],pathtokens);
							execv(bucket[0],bucket);
							fprintf(stderr, "couldnt execute %s\n", strerror(errno));
							exit(EXIT_FAILURE);

						}
						printf("pid %d\n", pid);
						processes = allocate(processes,processcount);
						processcount++;
						processes[processcount-1].pid = pid;
						processes[processcount-1].position = 1;
						processes[processcount-1].state = true;
						processes[processcount-1].cmd = (char *)malloc(256*sizeof(char));
						strcpy(processes[processcount-1].cmd, bucket[0]);
						int i = 1;
						while(bucket[i] != NULL){
							strcat(processes[processcount-1].cmd," ");
							strcat(processes[processcount-1].cmd, bucket[i]);
							i++;
						}
					}
				}

			}

			else {
				if(ifBackground(bucket, numI)){
					printf("Error: not a valid input\n");
				}
			}
		}	
		else if(isredirectchar(bucket)== true){
			redirect(bucket,pathtokens);
		}
		else if(shouldpipe(bucket)){
                        dopipe(bucket,pathtokens);
		} 


		else if(strcmp(bucket[0], "echo") == 0){
			if(bucket[1] != NULL){
				char tempChar[1]; char *tempS = bucket[1];
				char * ePath; memcpy(tempChar, bucket[1],1);
				if(tempChar[0] ==  '$'){ strcpy(tempS,tempS+1);
					if((ePath = getenv(tempS))){
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
				printf("Nothing to check if it is an environmental variable\n");
			}
		}
		else if(strcmp(bucket[0], "exit")==0){
			break;
		}
		else if(strcmp(bucket[0], "cd")==0){
			if(bucket[1] != NULL){
				if(bucket[2] == NULL){
					if(chdir(bucket[1])!= -1){
						continue;
					}
					else{
						printf("Error target is not a directory\n");
					}
				}
				else{
					printf("Error more than one argument present\n");
				}
			}
		} else if(strcmp(bucket[0], "io")==0){
			FILE *fp;
			char file_name[50];

			if ((pid = fork()) == 0){
				bucket[1] = addPath(bucket[1],pathtokens);
				//printf("%s\n",bucket[0]);                        
				//execute program av[0] with arguments av[0]... replacing this program
				//printTokens(new_bucket, numI-1);
				execv(bucket[1],&bucket[1]);
				//execv(bucket[1],&bucket[1]);
				/*fprintf(stderr, "can't execute %s\n", av[0]);*/
				exit(EXIT_FAILURE);
			}
			printf("pid %d\n",pid);
			//sprintf(file_name,"/proc/%d/io",pid);
			//char ch; //fp = fopen(file_name,"r");
			//while((ch = fgetc(fp)) != EOF){
			//    printf("%c",ch);
			//}
			//if(fp!=NULL){
			//printf("%s\n",fp);
			//}
			//wait(&pid);
			getpgid(pid);
			int rchar = 0;
			int wchar = 0;
			int syscr = 0;
			int syscw = 0;
			int read_bytes = 0;
			int write_bytes = 0;
			int cancelled = 0;
			while(true){
				char line[100];
				if(waitpid(pid, &status, WNOHANG) == 0){
					sprintf(file_name,"/proc/%d/io",pid);
					fp = fopen(file_name,"r");
					if(fp!=NULL){
						fgets(line,sizeof(line),fp);
						sscanf(line,"%*s %d",&rchar);
						fgets(line,sizeof(line),fp);
						sscanf(line,"%*s %d",&wchar);
						fgets(line,sizeof(line),fp);
						sscanf(line,"%*s %d",&syscr);
						fgets(line,sizeof(line),fp);
						sscanf(line,"%*s %d",&syscw);
						fgets(line,sizeof(line),fp);
						sscanf(line,"%*s %d",&read_bytes);
						fgets(line,sizeof(line),fp);
						sscanf(line,"%*s %d",&write_bytes);
						fgets(line,sizeof(line),fp);
						sscanf(line,"%*s %d",&cancelled);
						fclose(fp);
					}
				}else{
					printf("rchar: %d\n",rchar);
					printf("wchar: %d\n",wchar);
					printf("syscr: %d\n",syscr);
					printf("syscw: %d\n",syscw);
					printf("read_bytes: %d\n",read_bytes);
					printf("write_bytes: %d\n",write_bytes);
					printf("cancelled: %d\n",cancelled);
					break;
				}
			}

			//while ((w = wait(&status)) != pid && w != -1){
			//while((ch = fgetc(fp)) != EOF){
			//    printf("%c\n",ch);
			//}
			//     continue;          
			//}         
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

			//printTokens(bucket, numI);
			bucket[0] = addPath(bucket[0],pathtokens);
			//printTokens(bucket, numI);
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
	printf("\tSession time: %ds\n",(int)(finish.tv_sec-start.tv_sec));
	return 0;
}

/*reallocates instruction array to hold another token,
  returns new pointer to instruction array */
char** addToken(char** instr, char* tok, int numTokens)
{
        int i;
        
        char** new_arr;
        new_arr = (char**)malloc((numTokens+1) * sizeof(char*));				
        memset(new_arr, '\0', (numTokens+1)*sizeof(char));
        /*copy values into new array*/
        for (i = 0; i < numTokens; i++)
        {
                new_arr[i] = (char *)malloc((strlen(instr[i])+1) * sizeof(char));
                memset(new_arr[i], '\0', (strlen(instr[i])+1)*sizeof(char));
	        strcpy(new_arr[i], instr[i]);
                //printf("%s\n",new_arr[i]);
        }
        
        /*add new token*/
        new_arr[numTokens] = (char *)malloc((strlen(tok)+1) * sizeof(char));
        memset(new_arr[numTokens], '\0', (strlen(tok)+1)*sizeof(char));
        strcpy(new_arr[numTokens], tok);
        
        if (numTokens > 0)
        free(instr);
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
char * addPath(char * instr, char ** path){ int i;
       DIR *d;
       int tmp;
       char * tmp_command;
               //printf("instr: %s\n",instr);
       char * tmp_instr = (char*)malloc(100*sizeof(char));
       tmp = 0;
       i = 0;
       memset(tmp_instr, '\0', (100)*sizeof(char));
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
               free(tmp_instr);
               closedir(d);
               return NULL;
           }else{
                   struct dirent *dp;
		   while((dp = readdir(d))!= NULL){
			   if(strcmp(tmp_command,dp->d_name)==0){
                                   free(tmp_instr);
                                   closedir(d);
				   return instr;
			   }
		   }
           }
               printf("%s: Command not found.\n",instr);
               free(tmp_instr);
               closedir(d);
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
       i = 0;
       //add absolute path for execution
       while(path[i] != NULL){
            //printf("%s\n",path[i]);
            DIR *d;
            d = opendir(path[i]);
            if(d){
                struct dirent *dp;
                while((dp = readdir(d))!= NULL){
                    //printf("%s\n", dp->d_name);
                    if(strcmp(instr,dp->d_name)==0){
                       char * new_path = (char *)malloc(200*sizeof(char));
                       memset(new_path, '\0', (200)*sizeof(char));
                       strcpy(new_path,path[i]);
                       strcat(new_path,"/");
                       strcat(new_path,dp->d_name);
                       free(tmp_instr);
                       closedir(d);
                       return new_path;
                    }
                } }
            i++;
            closedir(d);
       }
       printf("%s: Command not found.\n",instr);
       free(tmp_instr);
       return NULL; 

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
        while(path[i]!='\0'){ //printf("%c\n",path[i]);
             if(path[i]==':'){ path[i]='\0';
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

struct queue* allocate(struct queue *processes, int num){
        int i = 0;
	struct queue *process = malloc((num+1)* sizeof(struct queue));
	for(i = 0; i < num; i++){
            process[i].pid = processes[i].pid;
            process[i].cmd = (char *)malloc((strlen(processes[i].cmd)+1) * sizeof(char));
            memset(process[i].cmd, '\0', (strlen(processes[i].cmd)+1)*sizeof(char));
            strcpy(process[i].cmd, processes[i].cmd);
            process[i].position = processes[i].position;
            process[i].state = processes[i].state;
        }
        //if(num > 0)
	//free(processes);
	return process;
}
bool ifBackground(char** instr, int num){
	int counter = 0;
	while(counter < num){
		if(strcmp(instr[counter],"&")==0){
			return true;
			printf("HERE");
		}
		counter=counter+1;
	} return false;
}

void redirect(char *bucket[],char ** pathtoken) {
	pid_t pid;
	int infile, outfile;
        int status;
	int i = 0;
        int w;
        bool new_in = false;
	printf("Redirect function begins\n");
	if ((pid = fork()) == -1){
		printf("Error: Child process could not be created\n");
		return (EXIT_FAILURE);
	}

	if (pid == 0){ 
		printf("Child Process begins\n");
		//Working in child process
		//loop through command line input
		while (bucket[i] != NULL){
			if (strcmp(bucket[i],">")==0) {
				//command is outputting to file (CMD > FILE)
				printf("OUTPUT START\n");
				if(bucket[i]== NULL || bucket[i-1] == NULL || bucket[i+1] == NULL){
					printf("Error: Not enough input arguments\n");
					return (EXIT_FAILURE);
				}
				else if (strcmp(bucket[i], ">") != 0 ){ 
					printf( "Expected '>' error\n");
					return (EXIT_FAILURE);
				}

				if ((outfile = open(bucket[i+1], O_WRONLY | O_CREAT | O_TRUNC , S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) ==1){
					fprintf(stderr, "shell: error creating file: %s\n", strerror(errno));
					return(EXIT_FAILURE);
				}	
				dup2(outfile, STDOUT_FILENO);
				close(outfile); 
                                bucket[i] = NULL;

			}

			else if (strcmp(bucket[i], "<")==0) { 
				//file is inputting to command (CMD < FILE)
                                bucket[i] = NULL;
				printf("file to open %s\n",bucket[i+1]);
				if(bucket[i+1] == NULL){ 
					printf("Error: Missing file name for redirect\n"); return EXIT_FAILURE;
				}
				if((infile = open(bucket[i+1], O_RDONLY )) == -1){
					fprintf(stderr, "shell: error opening file: %s\n", strerror(errno));
					return(EXIT_FAILURE);
				}
                                new_in = true;
		   dup2(infile, 0);
		   close(infile);
			}
			i++;
		}
		bucket[0] = addPath(bucket[0], pathtoken); 
                fprintf(stderr,"path %s\n",bucket[0]);
		execv(bucket[0], bucket);
	} 
	waitpid(pid, NULL, 0);	
}		
char * expandenv(char* env){ return getenv(env+1);
 }

void stillrunning(struct queue *processes, int processcount){
			int i; int j = 0; int w = 0;
			int status;
			int pid = 0;
			for(i = 0; i < processcount; i++){
				printf("pid %d, cmd %s state %d\n", processes[i].pid, processes[i].cmd, processes[i].state);
				 if(processes[i].state == true){ /*check if process is running */
           				if(waitpid(processes[i].pid, &status, WNOHANG) != 0){        
             					printf("[%d] [%s]\n",processes[i].position,processes[i].cmd);
                				processes[i].state = false;		
		}else{ 
		j++; //TODO needs to be removed later
		}
	}
}	
printf("Still running %d\n", j);
}

bool containsspecialchar(char ** bucket){
     int i;
     i = 0;
     while(bucket[i]!=NULL){
         if(strcmp(bucket[i],"|")==0 || strcmp(bucket[i],">")==0 || strcmp(bucket[i],"<")==0){
             return true;
         } 
     i++;
     }
     return false;
}

bool isredirectchar(char ** bucket){
	int i;
	i = 0;
	while(bucket[i]!= NULL){
		if(strcmp(bucket[i], ">") == 0 || strcmp(bucket[i], "<")==0){
			return true;
		}
	i++;
	}
	return false;
}
bool shouldpipe(char ** bucket){
     int i = 0;
     while(bucket[i]!=NULL){
           if(strcmp(bucket[i],"|") == 0){
              return true;
           }
           i++;
     }
     return false;
}
void dopipe(char ** bucket, char ** pathtokens){
     printf("made it to piping\n");
     int * pipes = (int*)malloc(20*sizeof(int));
     int i = 0;
     int numpipes = 0;
     int pid;
     pipes[0] = 0;
     while(bucket[i] != NULL){
         if(strcmp(bucket[i],"|") == 0){
            if(bucket[i+1] == NULL){
               printf("Invalid null command.\n");
               return;
            }
            numpipes++;
            pipes[numpipes] = i+1;
            bucket[i] = NULL;
         }
     i++;
     }
     bucket[i] = NULL;
     printf("pipes: %d\n",numpipes);
     int * current_pipe = NULL;
     int * old_pipe = NULL;
     int new_out;
     int new_in;
     int old_pid;
     for(i = 0; i <= numpipes; i++){
	     current_pipe = (int *)malloc(2*sizeof(int));
	     pipe(current_pipe);
	     if(i == 0) {
                     printf("current[0]: %d \n",current_pipe[0]);
                     printf("== 0\n");
		     if((pid = fork()) == 0){
                             //if((new_out = open(current_pipe[1], O_WRONLY | O_CREAT | O_TRUNC , S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1){
			     //if((new_out = fopen(current_pipe[1], "rw")) < 0){
			//	fprintf(stderr, "shell: error creating file: %s\n", strerror(errno));
			//	return(EXIT_FAILURE);
                          //   }
                             if(dup2(current_pipe[1],1) < 0){
                                printf("dup error\n");
                             }
			     close(current_pipe[1]);
			     close(current_pipe[0]);
                             printf("pipe[i]: %s\n",bucket[pipes[i]]);
                             printf("pipe[i+1]: %s\n",bucket[pipes[i]+1]);
			     bucket[0] = addPath(bucket[0],pathtokens);
                             execv(bucket[0],bucket);
		     }
		     else{
			     close(current_pipe[1]);
		     }
	     } 
	     else if(i != numpipes){
                     printf("!= numpipes \n");
		     if((pid = fork()) == 0){
			     //if((new_out = fopen(current_pipe[1],"wr")) == -1){
			     //   fprintf(stderr, "shell: error creating file: %s\n", strerror(errno));
			     //   return(EXIT_FAILURE);
                             //}
			     //if((new_in = fopen(old_pipe[0], "rw")) == -1){
			     //   fprintf(stderr, "shell: error creating file: %s\n", strerror(errno));
			     //   return(EXIT_FAILURE);
                             //}
                             if(dup2(current_pipe[1],1) < 0){
                                printf("dup error\n");
                             }
                             if(dup2(old_pipe[0],0) < 0){
                                printf("dup error\n");
                             }
			     close(current_pipe[0]);
			     close(current_pipe[1]);
			     close(old_pipe[0]);
			     close(old_pipe[1]);
                             printf("pipe[i]: %s\n",bucket[pipes[i]]);
                             printf("pipe[i+1]: %s\n",bucket[pipes[i+1]]);
			     bucket[pipes[i]] = addPath(bucket[pipes[i]],pathtokens);
                             execv(bucket[pipes[i]],&bucket[pipes[i]]);
		     }else{
			     close(old_pipe[0]);
			    // close(old_pipe[1]);
			     close(current_pipe[1]);
		     }
	     }else{
                     printf("last \n");
                     printf("old[0]: %d \n",old_pipe[0]);
                     if((pid = fork()) == 0){
                             //if((new_in = open(old_pipe[0], O_WRONLY | O_CREAT | O_TRUNC , S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) ==1){
                             //if((new_in = fopen(old_pipe[0],"wr")) == -1){
			//	fprintf(stderr, "shell: error creating file: %s\n", strerror(errno));
			//	return(EXIT_FAILURE);
                          //   }
                             if(dup2(old_pipe[0],0) < 0){
                                printf("dup error\n");
                             }
			     close(old_pipe[0]);
			     close(old_pipe[1]);
                             printf("pipe[i]: %s\n",bucket[pipes[i]]);
                             printf("pipe[i+1]: %s\n",bucket[pipes[i]+1]);
			     bucket[pipes[i]] = addPath(bucket[pipes[i]],pathtokens);
                             execv(bucket[pipes[i]],&bucket[pipes[i]]);
                            
                     }
	     }
             old_pid = pid;
             if(old_pipe != NULL){
                free(old_pipe);
             }
             old_pipe = current_pipe;
             current_pipe = (int *)malloc(2*sizeof(int));
     }
     free(current_pipe);
}
