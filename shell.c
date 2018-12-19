#include <sys/wait.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<signal.h>

#define rl_size 1024        // read line buffer size
#define tok_bufsize 64	  	// token buffer size
#define tok_delim " \t\r\n\a"	//token delimiter 
#define hist_size 10

int cur_pos=-1;
char *history[hist_size];

// declarations for built in commands
int sh_cd(char **args);
int sh_help(char **args);
int sh_exit(char **args);

// array of builtin commands
char *builtin_str[]={
	"cd", "help" ,"exit"
};


int (*builtin_func[])(char **)={
	&sh_cd,
	&sh_help,
	&sh_exit
};

// function to return size of buitin array
int num_builtins(){
	return sizeof(builtin_str)/sizeof(char *);
}

// builtin function implementation

int sh_cd(char **args){
	if(args[1] == NULL){
		fprintf(stderr, "Arguments missing" );

	}
	else{
		if(chdir(args[1]) != 0){
			perror("error");
		}
	}

 return 1;
}

int sh_help(char **args){
	int i;
	printf("Simran Batra's linux shell \n");
	printf("Type program names and arguments and press enter\n");
	printf("Append \"&\" after the arguments for concurrency between parent-child process.\n");
    printf("The following are built in:\n");

    for(i = 0; i < num_builtins(); i++){
        printf(" %s\n", builtin_str[i]);
    }

    printf("Use the man command for information on other programs.\n");
	return 1;	
}

int sh_exit(char **args){
	return 0;
}


// reading input
char *read_line(void){
	int buffer_size= rl_size;
	int position =0;
	char *buffer=malloc(sizeof(char) * buffer_size);
	int c;

	if(!buffer){
		fprintf(stderr, "Allocation Error \n" );
		exit(1);
	}

	while(1){
		c=getchar();

		if(c==EOF || c=='\n'){
			buffer[position]='\0';
			return buffer;
		}
		else
		{
			buffer[position]=c;
		}
		position++;

		if(position >= buffer_size){
			buffer_size += rl_size ;
			buffer = realloc(buffer,buffer_size);
			if(!buffer){
				fprintf(stderr, "Allocation Error \n" );
				exit(1);
			}
		}


	}

	return buffer;
}


// spliting input in command and arguments
char **split_line(char *line){
	int buffer_size=tok_bufsize;
	char **tokens = malloc(buffer_size*sizeof(char*));
	char *token;
	int position=0;

	if(!tokens){
		fprintf(stderr, "Allocation Error" );
		exit(EXIT_FAILURE);
	}

	token=strtok(line,tok_delim);

	while(token!=NULL){
		tokens[position]=token;
		position++;

		if (position>=buffer_size)
		{
			buffer_size += tok_bufsize;
			tokens=realloc(tokens , buffer_size*sizeof(char*));
			if(!tokens){
			fprintf(stderr, "Allocation Error" );
			exit(EXIT_FAILURE);
			}
		}

		token=strtok(NULL,tok_delim);

	}
	tokens[position]=NULL;
	return tokens;
}


int launch(char **args){
	pid_t pid ,wpid;
	int status;

	pid=fork();

	if(pid==0){ //child process
			signal(SIGINT, SIG_DFL);
			if(execvp(args[0],args)==-1)
				perror("Error");
			exit(EXIT_FAILURE);
	}

	else if(pid <0){ // error forking
		perror("Error");
	}

	else{ // parent process
		
		do{
			wpid=waitpid(pid,&status,WUNTRACED);
		}while(!WIFEXITED(status) && !WIFSIGNALED(status));
	}


return 1; // 1 signal calling function to again prompt for input

}

int sh_history(char **args );

int execute(char **args , char *line){
	int i;
	if(args[0]==NULL){return 1;}

	cur_pos = (cur_pos + 1) %hist_size;
	history[cur_pos] = strdup(line);
	//printf("%s %d\n",history[cur_pos],cur_pos );

	 if(strcmp(args[0], "history") == 0 ||
             strcmp(args[0], "!!") == 0 || args[0][0] == '!'){
        return sh_history(args);
	}

	for (int i = 0; i < num_builtins(); ++i)
	{
		if (strcmp(args[0],builtin_str[i])==0)
		{
			return (*builtin_func[i])(args);
		}
	}

	return launch(args);
}

// history of commands
int sh_history(char **args){

	if(cur_pos==-1 || history[cur_pos]==NULL){
		fprintf(stderr,"No commands in history1\n");
		return 1;//exit(EXIT_FAILURE);
	}

	if(strcmp(args[0],"history")==0){
		int first_pos=0,pos,count=0;

		if(cur_pos!=hist_size && history[cur_pos+1]!=NULL)
			{  first_pos=cur_pos+1;
				count=hist_size;
			}
		else count=cur_pos-first_pos+1;	

		int k=0;
		pos=first_pos;
		while(k<count){
			char *command=history[pos];
			printf("%d %s\n",k+1,command );
			pos=(pos+1)%hist_size;
			k++;

		}
	}

	else if(strcmp(args[0],"!!")==0){
		char **cmd_args;
		char *last_command;
		int pos=(cur_pos-1+hist_size)%hist_size;
		if(cur_pos==0)
			{
				fprintf(stderr,"No commands in history\n");
				return 1;//exit(EXIT_FAILURE);
			}

		last_command=history[pos];
		cmd_args=split_line(last_command);
		return execute(cmd_args , last_command);

	}

	else if(args[0][0]=='!'){
		if(args[0][1] == '\0'){
            fprintf(stderr, "Expected arguments for \"!\"\n");
            return 1;//exit(EXIT_FAILURE);
		}

		int offset=args[0][1] - '0';

		if(cur_pos!=hist_size && history[cur_pos+1]!=NULL)
			offset=(cur_pos+offset)%hist_size;
		else offset--;
		
		if (history[offset]==NULL)
			{
				fprintf(stderr, "No such command in history\n" );
				return 1;//exit(EXIT_FAILURE);
			}	

		char *command=history[offset];
		char **cmd_args=split_line(command);

		return execute(cmd_args ,command);	
		
	}
	else
	{
		perror("Error");
		exit(EXIT_FAILURE);
	}
}


void shell_loop(void){
	char *line;
	char **args;
	int status;

	do{
		printf("linuxsh>");
		line=read_line();
		args=split_line(line);
		status=execute(args ,line);

		free(line);
		free(args);

	}while(status);

}
int main(int argc, char const *argv[])
{	
	signal(SIGINT, SIG_IGN);
	shell_loop();


	return 0;
}

