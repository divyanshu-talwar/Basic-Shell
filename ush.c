#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
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
int open_outfile();
int open_infile();

int piping, index_inp, index_out, inp_redir, out_redir;
char* infile;
char* outfile;
int infd, outfd;

char *builtins[] = {"cd","exit","help","history"};
int (*builtin_function[]) (char **) = { &ush_cd, &ush_exit, &ush_help, &ush_history };

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

char * split_redirection(char *line){
	int token_buffer_size = TOKEN_SIZE;
	char **tokens = malloc( token_buffer_size * sizeof(char *));
	char *token;
	infile = outfile = NULL;
	int pos = 0;
	if( !tokens){
		fprintf(stderr, "allocation error\n" );
		exit(0);
	}

	if(inp_redir == 1 && out_redir == 1) {
	        token = strtok(line, "<>");
	        pos = 0;
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
				token = strtok(NULL,"<>");
			}
	        if(index_inp < index_out ) {
	                infile = strdup(tokens[pos - 2]);
	                outfile = strdup(tokens[pos - 1]);
	        }
	        else {
	                infile = strdup(tokens[pos - 1]);
	                outfile = strdup(tokens[pos - 2]);
	        }
	        infile = strtok(infile," \n");
	        outfile = strtok(outfile," \n");
	        tokens[pos - 2] = tokens[pos - 1] = NULL;
	        
	        return tokens[0];
	}
	        
	else if(inp_redir == 1) {
	        token = strtok(line, "<");
	        pos = 0;
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
				token = strtok(NULL,"<");
			}
	        infile = strdup(tokens[pos - 1]);
	        infile = strtok(infile," \n");
	        return tokens[0];   
	}
	else if(out_redir == 1) {
	        token = strtok(line, ">");
	        pos = 0;
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
				token = strtok(NULL,">");
			}
	        outfile = strdup(tokens[pos - 1]);
	        outfile = strtok(outfile," \n");
	        return tokens[0];   
	}
	else{
		return NULL;
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
		if(outfile){
			open_outfile();
		}
		if(infile){
			open_infile();
		}
		if(!run_builtin_command(args)){
			int error_code = execvp(args[0],args);
			if(error_code == -1){
				fprintf(stderr, "ush: command not found \n" );
			}
			exit(1);
		}
		else{
			exit(1);
		}
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
			if(i == 0){
				if(infile){
					infd = open_infile();
				}
			}
			if(args_arr[i+1] != NULL){
				dup2(pipe_channel[1],1);
			}
			else if(args_arr[i+1] == NULL){
				if(outfile){
					outfd = open_outfile();
				}
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

	for (i = 2; i < size; i++) {
    	if( strcmp(args[0],builtins[i]) == 0){
    		return (*builtin_function[i])(args);
    	}
	}
	return 0;
}

int execute(char **args){
	int i;
	int size = 2;

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

int check_special_characters(char* line) {
        int i;
        outfile = NULL;
		infile = NULL;
        index_inp = index_out = piping = inp_redir = out_redir = 0;
        for( i = 0 ; line[i] ; i++) {
                if(line[i] == '|') {
                        piping = 1;
                }
                if(line[i] == '<') {
                        inp_redir = 1;
                        if(index_inp == 0 ){
                        	index_inp = i;
                        }
                }
                if(line[i] == '>') {
                        out_redir = 1;
                        if(index_out == 0 ){
                        	index_out = i;
                        }
                }
        }
        if(piping){
        	return 1;
        }
        else{
        	return -1;
        }
}

int open_infile() {
        int f = open(infile, O_RDONLY, S_IRWXU);
        if (f < 0) {
                perror(infile);
                
        }
        dup2(f, 0);
        close(f);
        return f;
}

int open_outfile() {
        int f;
        f = open(outfile, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
        if(f < 0) {
                perror(outfile);
        }
        dup2(f,1);
        close(f);
        return f;
}

void ush_shell(FILE *input){
	char *line;
	char **args;
	char ***args_arr;
	int argc = 0;
	int status;
	infd = 0;
	outfd = 1;

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
		if(check_special_characters(line) == -1){
			if(out_redir || inp_redir){
				line = split_redirection(line);
			}
			args = split_line(line,&argc);
			status = execute(args);
			free(line);
			free(args);
		}
		else{
			if(out_redir || inp_redir){
				line = split_redirection(line);
			}
			args_arr = split_pipe(line,&argc);
			run_pipe_command(args_arr);
			free(args_arr);
			status = 1;
		}
		argc = 0;

	}while(status);

}

int main(int argc, char const *argv[]){
	FILE *input;
	input = fopen(".ush_history","a+");
	history_queue = initialize_queue();
	history_read(input);
	ush_shell(input);
	fclose(input);
	return 0;
}
