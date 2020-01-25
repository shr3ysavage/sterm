#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<readline/readline.h>
#include<readline/history.h>

// Macro to clear the shell using escape sequences
#define clear() printf("\033[H\033[J")
// Macros for maximum characters and maximum commands
#define MAXCHAR 1000
#define MAXLIST 100

// Greet user
void greet();

// Display prompt. Await and take input
int prompt(char *str);

// Execute system commands
void system_exec(char **parsed);

// Execute piped system commands
void system_exec_piped(char **parsed, char **parsed_pipe);

// Print help message
void help();

// Execute built-in commands
int built_in_exec(char **parsed);

// Parse pipes
int parse_pipe(char *str, char **strpiped);

// Parse command arguments
void parse_space(char *str, char **parsed);

int process_string(char *str, char **parsed, char **parsedpipe);

int main() {

	char input_string[MAXCHAR], *parsed_args[MAXLIST];
	char *parsed_args_piped[MAXLIST];
	int exec_flag = 0;

	// Greet user
	greet();

	while(1) {
		// Display prompt and take input
		if(prompt(input_string))
			continue;
		// exec_flag = 0 -> no command; 1 -> simple command; 2 -> piped commands
		exec_flag = process_string(input_string, parsed_args, parsed_args_piped);

		if(exec_flag == 1)
			system_exec(parsed_args);

		if(exec_flag == 2)
			system_exec_piped(parsed_args, parsed_args_piped);
	}

	return 0;
}

void greet() {
	clear();
	puts("sterm\n");
	puts("Shell for Linux\n");
	puts("Author - Shreyansh Singh\n");

	// Get username from environment variable
	char *username = getenv("USER");
	printf("Logged in as: %s\n", username);
	sleep(3);
	clear();
}

int prompt(char *str) {
	// Store command from user
	char *command;

	// Get username from environment variable
	char *username = getenv("USER");

	// Get current working directory
	char pwd[1024];
	getcwd(pwd, sizeof(pwd));

	// Show username and present working directory, then await command from user
	printf("%s [%s]", username, pwd);
	command = readline(": ");

	// If length of command is non-zero
	if(strlen(command) != 0) {
		// Add command in history file
		add_history(command);

		// Copy user's command to given string for further operations
		strcpy(str, command);

		// Exit status (without error)
		return 0;
	} else {
		// Exit status (with error)
		return 1;
	}
}

void system_exec(char **parsed) {
	// Create child process for command execution
	pid_t pid = fork();

	// Error handling
	if(pid == -1) {
		fprintf(stderr, "Error creating command process\n");
		return;
	} else if(pid == 0) {
		// Provide array of strings as argument list of command
		if(execvp(parsed[0], parsed) < 0) {
			printf("Couldn't execute command\n");
			exit(0);
		}
	} else {
		// Wait for child to terminate
		wait(NULL);
		return;
	}
}

void system_exec_piped(char **parsed, char **parsed_pipe) {
	// File descriptors for pipe; 0 -> read end; 1 -> write end
	// First child writes at the write end, second child reads from the read end
	int pipes[2];
	pid_t p1, p2;

	if(pipe(pipes) < 0) {
		fprintf(stderr, "Couldn't initialize pipe\n");
		return;
	}

	p1 = fork();
	if(p1 < 0) {
		fprintf(stderr, "Couldn't create child for pipe\n");
		return;
	}

	if(p1 == 0) {
		// Child 1 executing
		// Write at the write end
		close(pipes[0]);
		// Create copy of file descriptor to STDOUT_FILENO
		dup2(pipes[1], STDOUT_FILENO);
		// Close file descriptor
		close(pipes[1]);

		// Execute command with arguments and handle error
		if(execvp(parsed[0], parsed) < 0) {
			printf("Couldn't execute first command\n");
			exit(0);
		}
	}
	else {
		// Parent executing
		p2 = fork();

		if(p2 < 0) {
			printf("Couldn't fork\n");
			return;
		}

		if(p2 == 0) {
			// Child 2 executing
			// Read at the read end
			close(pipes[1]);
			// Create copy of file descriptor to STDIN_FILENO
			dup2(pipes[0], STDIN_FILENO);
			// CLose file descriptor
			close(pipes[0]);

			// Execute commands with arguments and handle error
			if(execvp(parsed_pipe[0], parsed_pipe) < 0) {
				printf("Couldn't execute second command\n");
				exit(0);
			}
		}
		else {
			// Parent executing. Wait for two children
			wait(NULL);
			wait(NULL);
		}
	}
}

void help() {
	puts("sterm\n");
	puts("Author - Shreyansh Singh\n");
	puts("Copyright - Copy what you can\n");
	puts("A terminal for Linux\n");
	puts("Happy hacking\n");
}

int built_in_exec(char **parsed) {
	int built_in = 4;
	char *built_in_commands[built_in];
	built_in_commands[0] = "exit";
	built_in_commands[1] = "quit";
	built_in_commands[2] = "hello";
	built_in_commands[3] = "help";

	int arg = 0;
	for(int i = 0; i < built_in; ++i) {
		if(strcmp(parsed[0], built_in_commands[i]) == 0) {
			arg = i + 1;
			break;
		}
	}

	switch(arg) {
		case 1:
			puts("Bye\n");
			exit(0);
		case 2:
			puts("Bye\n");
			exit(0);
		case 3:
			puts("Hello\n");
			return 1;
		case 4:
			help();
			return 1;
		default:
			break;
	}
}

int parse_pipe(char *str, char **strpiped) {
	// Seperate commands
	for(int i = 0; i < 2; ++i) {
		strpiped[i] = strsep(&str, "|");
		if(strpiped[i] == NULL)
			break;
	}

	// Return values: 0 -> pipe not found; 1 -> pipe found
	if(strpiped[1] == NULL)
		return 0;

	return 1;
}

void parse_space(char *str, char **parsed) {
	for(int i = 0; i < MAXLIST; ++i) {
		parsed[i] = strsep(&str, " ");

		if(parsed[i] == NULL)
			break;
		if(strlen(parsed[i]) == 0)
			--i;
	}
}

int process_string(char *str, char **parsed, char **parsed_pipe) {
	char *strpiped[2];
	int is_piped;

	// See if pipes exist, and store piped commands in strpiped
	is_piped = parse_pipe(str, strpiped);

	if(is_piped) {
		parse_space(strpiped[0], parsed);
		parse_space(strpiped[1], parsed_pipe);
	} else {
		parse_space(str, parsed);
	}

	if(built_in_exec(parsed))
		return 0;

	return 1 + is_piped;
}