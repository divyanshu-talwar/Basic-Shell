#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <readline/history.h>

#define BUFF_SIZE 1024
#define TOKEN_SIZE 128


int ush_cd(char **args);
int ush_help(char **args);
int ush_exit(char **args);

char *builtins[] = {"cd","help","exit"};
int (*builtin_function[]) (char **) = { &ush_cd, &ush_help, &ush_exit };

void sigintHandler(int sig_num)
{
    printf("\n Ctrl+C \n");
    printf("ush> ");
}

void sigHandler(int sig_num){
    printf("\n Ctrl+C \n");
}
int num_builtins(){
	return sizeof(builtins) / sizeof(char *);
}

int ush_cd(char **args){
	if(args[1] == NULL){
		fprintf(stderr, "expected argument to the cd command\n");
	}
	else{
		int stat = chdir(args[1]);
		if(stat != 0){
			fprintf(stderr, "Directory not found\n");
		}
	}
	return 1;
}

int ush_help(char **args){
  int i;
  int size = num_builtins();
  printf("Enter the command names and arguments, and press Enter to execute.\n");
  printf("Built in functions:\n");
  for (i = 0; i < size; i++) {
    printf("  %s\n", builtins[i]);
  }
  return 1;
}

int ush_exit(char **args){
	return 0;
}

void print(char *arr[], int size){
	int j = 0;
	for(j = 0; j<size; j++){
		printf("%s ",arr[j]);
	}
	printf("\n");
}

char** split_line (char * line, int *argc){
	int token_buffer_size = TOKEN_SIZE;
	char **tokens = malloc( token_buffer_size * sizeof(char *));
	char *token;
	if(line == NULL){
		return NULL;
	}
	if( strcmp(line,"\n") == 0){
		return NULL;
	}
	if( !tokens){
		fprintf(stderr, "allocation error\n" );
		exit(0);
	}

	else{
		token = strtok(line," ");
		int pos = 0;
		while(token != NULL){
			tokens[pos] = token;
			pos ++;

			if(pos > token_buffer_size){
				token_buffer_size += BUFF_SIZE;
				tokens = realloc(tokens, token_buffer_size);
				if(!tokens){
					fprintf(stderr, "allocation error\n");
					exit(0);
				}
			}

			if(token[strlen(token)-1] == '\n'){
				token[strlen(token)-1] = '\0';
			}
			token = strtok(NULL," ");
		}

		tokens[pos] = NULL;
		*argc = pos;
		return tokens;
	}
}

int run_command (char **args){
	signal(SIGINT, sigHandler);
	pid_t pid;
	int status;

	pid = fork();
	if( pid < 0){
		fprintf(stderr, "%s\n","fork failed" );
	}

	else if (pid == 0){
		int error_code = execvp(args[0],args);
		if(error_code == -1){
			fprintf(stderr, "ush: command not found \n" );
		}
		exit(0);
	}

	else{
		wait(NULL);
	}
}

int execute(char **args){

	int i;
	int size = num_builtins();
	if(args == NULL){
		return 1;
	}
	if(args[0] == NULL){
		return 1;
	}
	for (i = 0; i < size; i++) {
    	if( strcmp(args[0],builtins[i]) == 0){
    		return (*builtin_function[i])(args);
    	}
	}

	return run_command(args);
}
void ush_shell(void){
	char *line;
	char **args;
	int argc = 0;
	int status;

	do{
		signal(SIGINT, sigintHandler);
		line = readline("ush> ");
		if(line == NULL){
			printf("exit\n");
			break;
		}
		args = split_line(line,&argc);
		status = execute(args);


		free(line);
		free(args);
		argc = 0;
	}while(status);

}

int main(int argc, char const *argv[])
{
	ush_shell();
	return 0;
}
