#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define O_RDONLY 0x0000
#define true 1
#define false 0

void prepareCommand(char* command);
void executePipe(char **argT);

int totalNumberOfCommands = 0;

int ShellStatus = true;

int background = false;

struct Command_struct {
	char argsArray[10][50];
	int background;
};

void commandTokeniser(char *line, char **argv)
{
	while (*line != '\0')
	{
		while (*line == ' ' || *line == '&' || *line == '\t' || *line == '\n')
		{

			*line++ = '\0';
		}
		*argv++ = line;

		while (*line != '\0' && *line != ' ' && *line != '\t' && *line != '\n')
			
			line++;
			
	}

	argv = '\0';
}

void tokenize(char *line, char **argv, char token) {
	while (*line != '\0')
	{
		while (*line == token)
		{
			*line++ = '\0';
		}
		*argv++ = line;
		while (*line != '\0'  && *line != '\t' && *line != '\n' && *line != token)
			line++;
	}

	argv = '\0';
}

int strLength(char* str) {
	int i;
	for (i = 0; str[i] != '\0' ; ++i);
	return i;
}

char* strCat(char* a, char* b) {

	int i;
	char* concatStr = malloc((strLength(a) + strLength(b)) * 1);
	for (i = 0; a[i] != '\0'; ++i) {
		concatStr[i] = a[i];
	}

	for (int j = 0; b[j] != '\0'; j++) {
		concatStr[i++] = b[j];
	}
	return concatStr;
}

int stringCompare(char *a, char *b) {
	while (*b && *a == *b) {
		a++;
		b++;
	}
	return *a - *b;
}

void echo(char* variable) {
	if (variable[0] == '$') {
		char* var_no_dollar;
		var_no_dollar = &(variable[1]);
		printf("%s\n", getenv(var_no_dollar));
	}
	else {
		printf("%s\n", variable);

	}
}

void exportPath(char* args[]) {
	char* var;
	char* val;
	char *argv[64];
	tokenize(args[1], argv , '=');

	var = argv[0];
	val = argv[1];

	val = &(val[1]);
	val[strLength(val) - 1] = '\0';
	setenv(var, val, 1);
}


int cd(char* dir) 
{
	if(!background)
	return chdir(dir);
}

void executeCommand(char *argv[], int isBackground) {
	int status;
	int pid = fork();
	if (pid < 0) {
		printf("Error: Unable to fork child process");
		exit(1);
	}
	if (pid == 0) {
		int ret = execvp(argv[0], argv);
		if (ret < 0) {
			printf("Error: command executation failed");
			exit(1);
		}
	}
	else
	{
		if (!isBackground)
		{
			//printf("waiting .... \n");
			while (wait(&status) != pid);
		}
	}
}

void executePipe(char **argT){
	int i;
	int pid;
	
	for( i=1; i<totalNumberOfCommands; i++)
	{
		char *args[64] = {NULL};
		commandTokeniser(argT[i-1], args);
		
		if (stringCompare(args[0], "cd") == 0) {
			cd(args[1]);
		}
		else{
			//printf("\nHandling command number %d\n", i);
			int pd[2];
			pipe(pd);
			
			if (!fork()) {
				dup2(pd[1], 1);						
				execvp(args[0], args);
				//perror("exec");
				//abort();
			}
			dup2(pd[0], 0);
			close(pd[1]);
		}
	}
	char *args[64] = {NULL};
	commandTokeniser(argT[i-1], args);
	
	if (stringCompare(args[0], "cd") == 0) {
		cd(args[1]);
	}
	else{

		
		execvp(args[0], args);
		
	}
}

void print(char *string){
	int i = 0;
	while(string[i]){
		putc(string[i],stdout);	
		i++;
	}
}

int main(int argc, char *argv[], char *envp[]) {
	//int status = true;
	if(argv[1] != NULL) {
		int fd;
		char buffer[30];
		int i=0;
		if((fd = open(argv[1], O_RDONLY))!=-1) {
			while(read(fd, &buffer[i], 1)==1) {
				if(buffer[i]!='\n') {
					i++;
				}
				else if(i!=0){
					if(buffer[i]=='#') {
						printf("Found # \n");
					}
					buffer[i]='\0';
					i=0;
					prepareCommand(buffer);
				}
			}
		}
		else {
			printf("Unable to execute command as there was problem in opening the requested file");
		}
		exit(0);
	}
	do {
		char *argT[64] = {NULL};
		char input[1024];
		char cwd[1024];
		if((getcwd(cwd, sizeof(cwd)))!= NULL){
			char *toPrint = strcat(cwd, "@sbush> ");
			//printf("%s", toPrint);
			print(toPrint);
		}
		else
			printf("sbush> ");
		//scanf(" %[^\n]%*c",input);

		/* get input using getc */
		int input_counter = 0;
		char c;
		c = getc(stdin);
		
		while (c != '\n')
		{
			if (c=='&')
			{
				background = true;
				input[input_counter-1] = '\0';
			}
			else
			{

				input[input_counter] = c;
			}
			input_counter++;
			c = getc(stdin);


		}
		input[input_counter] = '\0';
		/* done getting input */
		
		tokenize(input, argT, '|');
		int counter = 0;
		while (argT[counter])
		{
			counter++;
		}

		totalNumberOfCommands = counter;
		counter -= 1;
		//printf("Commads found = %d\n", totalNumberOfCommands);
		
		if (totalNumberOfCommands == 1){
			prepareCommand(argT[0]);
		}
		
		else{
			int status = 0;
			int pid_parent = fork();
			if(pid_parent == 0)
			{
				executePipe(argT);
			}
			else{
				while (wait(&status) != pid_parent);
			}
		}
		
	} while (ShellStatus);
	return 0;
}




void prepareCommand(char* command) {
	char *args[64] = {NULL};
	commandTokeniser(command, args);

	if (stringCompare(args[0], "cd") == 0) {
		cd(args[1]);
	}
	else if (stringCompare(args[0], "export") == 0) {
		exportPath(args);
	}
	
	else if (stringCompare(args[0], "exit") == 0) {
		ShellStatus = false;
	}
	
	else if (stringCompare(args[0], "echo") == 0) {
		echo(args[1]);
	}
	else 
	{
		//printf("value of background : %d\n", background );
		executeCommand(args, background);
	}
}