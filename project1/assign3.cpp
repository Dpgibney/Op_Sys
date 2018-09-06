#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>
#include <string.h>
#include <cstddef>
void tokenizer(char [],char *[], char, int &);
int main()
{
	char input[256];
	int fdout, fdin;
	while(1){
		std::cin.getline(input,256);
		char *av[10] = {NULL};
		int ac = 0;
		tokenizer(input, av, ' ', ac);	
		if(strcmp(input,"exit")!=0){
			if(av[0]!=NULL){
			bool newout, newin = false;
			//call tokenizer function
			if(strcmp(av[0],"cd")==0){
				chdir(av[1]);
			}else{
				int pid; // process id
				int status; // child process exit status
				int w;
				//check if output is redirected
				for(int i = 0; i < ac; i++){
					if(strcmp(av[i],">")==0){
						//open file and check if it was successful
						fdout = (open(av[i+1], O_WRONLY | O_CREAT | O_TRUNC,
									S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
						if(fdout == -1){
							printf("Couldn't open file to write \n");
						}else{
							newout = true;
							for(int j = i; j < ac-2; j++){
								av[j] = av[i+2];
								i++;
							}
							//remove redirect from arguments
							ac-=2;
							av[ac] = NULL;
							break;
						}

					}
				}
				//check to see if input has been redirected
				for(int i = 0; i < ac; i++){
					if(strcmp(av[i],"<")==0){
						//open file and check if it was successful
						fdin = (open(av[i+1], O_RDONLY));
						if(fdin == -1){
							printf("Couldn't open file to read in \n");
						}else{
							newin = true;
							for(int j = i; j < ac-2; j++){
								av[j] = av[i+2];
								i++;
							}
							//remove redirect from arguments
							ac-=2;
							av[ac] = NULL;
							break;
						}
					}
				}
				void (*istat)(int), (*qstat)(int);
				ac++;
				int tty = open("/dev/tty", O_RDWR); // open tty for read/write;
				if (tty == -1)
				{
					fprintf(stderr, "can't open /dev/tty\n");
					exit(EXIT_FAILURE);
				}
				if ((pid = fork()) == 0)
				{
					// this is the forked child process that is a copy of the running program
					if(newin == false){
						dup2(tty, 0); // force stdin from tty
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
					// last argument must be NULL for execvp()
					av[ac] = NULL;
					// execute program av[0] with arguments av[0]... replacing this program
					execvp(av[0], av);
					fprintf(stderr, "can't execute %s\n", av[0]);
					exit(EXIT_FAILURE);
				}
				close(tty);
				if(newout == true){
					close(fdout);
				}
				if(newin == true){
					close(fdin);
				}
				// disable interrupt (^C and kill -TERM) and kill -QUIT
				istat = signal(SIGINT, SIG_IGN);
				qstat = signal(SIGQUIT, SIG_IGN);
				// wait until forked child process terminated, get its exit status
				while ((w = wait(&status)) != pid && w != -1)
					continue;
				if (w == -1)
					status = -1;
				// restore interrupt and quit signals
				signal(SIGINT, istat);
				signal(SIGQUIT, qstat);
			}
			}else{
			printf("enter input \n");
			}
		}else{
			exit(EXIT_SUCCESS);
		}
	}
}

void tokenizer(char str[], char *ptrs[], char delimiter, int & count){
	char *start = &str[0];
	bool parenth = false;
	int j = 0;
	int length = 0;
	for(int i = 0; str[i]!='\0'; i++){
		//parse input string
		if(str[i] == '"' && parenth == false){
			parenth = true;
			start = &str[i+1];
		}
		else if(str[i] == '"' && parenth == true){
			str[i] = '\0';
			ptrs[j] = start;
			j++;
			start = &str[i];
			parenth = false;
		}
		else if(str[i+1] == '\0' && str[i] != delimiter){
			str[i+1] = '\0';
			str[i+2] = '\0';
			i++;
			ptrs[j] = start;
			j++;
		}
		else if((str[i] == delimiter) && parenth == false && str[i-1] != delimiter && str[i-1] != '\0'){
			str[i] = '\0';
			ptrs[j] = start;
			j++;
			start = &str[i+1];
		}
		else if((str[i] == delimiter) && (str[i-1] == delimiter || str[i-1] == '\0') && parenth == false){
			start = &str[i+1];
		}
		length++;
	}
	ptrs[j] = NULL;
	count = j;
}
