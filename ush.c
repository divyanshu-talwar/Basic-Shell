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

struct node{
	char data[1000];
	struct node *next;
};

struct Queue{
	struct node *start, *end;
};

struct Queue * history_queue;

int ush_cd(char **args);
int ush_help(char **args);
int ush_exit(char **args);
int ush_history(char **args);
int run_builtin_command (char **args);

char *builtins[] = {"cd","help","exit","history"};
int (*builtin_function[]) (char **) = { &ush_cd, &ush_help, &ush_exit, &ush_history };

void sigintHandler(int sig_num){
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

struct node * create_node(char *line){
	struct node *temp = (struct node *) malloc(sizeof( struct node ));
	strcpy(temp -> data,line);
	temp -> next = NULL;
	return temp;
}

struct Queue * initialize_queue(){
	struct Queue *q = (struct Queue*) malloc(sizeof(struct Queue));
	q -> start = q -> end = NULL;
	return q;
}

void enqueue(struct Queue * q,char *line){
	struct node *temp = create_node(line);
	if(q -> start == NULL){
		q -> start = q -> end = temp;
	}
	else{
		q -> end -> next = temp;
		q -> end = temp;
	}
}

void ush_history_add(FILE *input, char *line){
	enqueue(history_queue, line);
	line = strcat(line,"\n");
	fputs(line,input);
}

void history_read(FILE *input){
	char *line = NULL;
	size_t len;
	while (1){
		if(getline(&line, &len, input) == -1){
			break;
		}
		if(strcmp(line,"\n")==0){
			continue;
		}
        if(line[strlen(line)-1] == '\n'){
            line[strlen(line) - 1] = '\0';           
        }
        enqueue(history_queue,line);
        add_history(line);
	}
}

void print_queue(struct Queue * q){
	struct node *temp = (struct node *)malloc(sizeof(struct node));
	temp = q -> start;
	while(temp -> next != NULL){
		printf("%s\n",temp -> data );
		temp = temp -> next;
	}
}

int ush_history(char **args){
  int i;
  int size = num_builtins();
  printf("History :\n");
  print_queue(history_queue);
  return 1;
}

int ush_exit(char **args){
	return 2;
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
				token_buffer_size += TOKEN_SIZE;
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

char*** split_pipe (char * line, int *argc){
	int token_buffer_size = TOKEN_SIZE;
	char **tokens = malloc( token_buffer_size * sizeof(char *));
	char *token;

	if( !tokens){
		fprintf(stderr, "allocation error\n" );
		exit(0);
	}

	else{
		token = strtok(line,"|");
		int pos = 0;
		while(token != NULL){
			tokens[pos] = token;
			pos ++;

			if(pos > token_buffer_size){
				token_buffer_size += TOKEN_SIZE;
				tokens = realloc(tokens, token_buffer_size);
				if(!tokens){
					fprintf(stderr, "allocation error\n");
					exit(0);
				}
			}

			if(token[strlen(token)-1] == '\n'){
				token[strlen(token)-1] = '\0';
			}
			token = strtok(NULL,"|");
		}
		*argc = pos;
		int i;
		char ***args_arr = (char*** )(malloc((pos + 1) * sizeof(char **)));
		for (i = 0; i < pos; i++){
			int a;
			args_arr[i] = split_line(tokens[i],&a);
		}
		args_arr[pos] = NULL;
		return args_arr;
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

int run_pipe_command(char ***args_arr){
	int pipe_channel[2];
	int prev_filedes = 0;
	pid_t pid;
	int i = 0;

	while(args_arr[i] != NULL){
		if(pipe(pipe_channel) == -1){
			exit(1);
		}

		pid = fork();
		if(pid < 0){
			exit(1);
		}
		else if(pid == 0){
			dup2(prev_filedes,0);
			if(args_arr[i+1] != NULL){
				dup2(pipe_channel[1],1);
			}
			close(pipe_channel[0]);
			if(!run_builtin_command(args_arr[i])){
				int error_code = execvp(args_arr[i][0], args_arr[i]);
				if(error_code == -1){
					fprintf(stderr, "ush: command not found \n" );
					exit(0);
				}
				exit(0);
			}
			else{
				exit(0);
			}
		}
		else{
			wait(NULL);
			close(pipe_channel[1]);
			prev_filedes = pipe_channel[0];
			i++;
		}
	}
}

int run_builtin_command (char **args){
	int i;
	int size = num_builtins();

	for (i = 0; i < size; i++) {
    	if( strcmp(args[0],builtins[i]) == 0){
    		return (*builtin_function[i])(args);
    	}
	}
	return 0;
}

int execute(char **args){

	if(args == NULL){
		return 1;
	}
	if(args[0] == NULL){
		return 1;
	}
	int stat = run_builtin_command(args);
	if(!stat){
		return run_command(args);
	}
	else if(stat == 2){
		return 0;
	}
	return 1;
}

void ush_shell(FILE *input){
	char *line;
	char **args;
	char ***args_arr;
	int argc = 0;
	int status;

	do{
		signal(SIGINT, sigintHandler);
		line = readline("ush> ");
		if(line == NULL){
			printf("exit\n");
			break;
		}
		if( strcmp(line,"\0") != 0){
			add_history(line);
			ush_history_add(input,line);
		}
		if(strstr(line,"|") == NULL){
			args = split_line(line,&argc);
			status = execute(args);
			free(line);
			free(args);
		}
		else{
			args_arr = split_pipe(line,&argc);
			run_pipe_command(args_arr);
			free(args_arr);
			status = 1;
		}

		argc = 0;
	}while(status);

}

int main(int argc, char const *argv[])
{
	FILE *input;
	input = fopen(".ush_history","a+");
	history_queue = initialize_queue();
	history_read(input);
	ush_shell(input);
	fclose(input);
	return 0;
}
