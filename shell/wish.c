#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"


// built-ins: exit, cd, path

bool is_interactive_mode(int argc) {
    if (argc == 1) {
        return true;
    }
    return false;
}

void print_ps1() {
    fprintf(stderr, "wish> ");
}

void print_error() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

char *wish_read_line() {
    char *line = NULL;
    size_t buffsize = 0;
    if (getline(&line, &buffsize, stdin) == -1) {
        if (feof(stdin)) {
            exit(EXIT_SUCCESS);
        } else {
            print_error();
            exit(EXIT_FAILURE);
        }
    }
    return line;
}

// TODO: should this return a linked list instear of an array of char arrays?
void wish_tokenize_line(char *line) {
    return line;
}

void with_execute() {
    int ret = fork();
    if (ret == 0) {
        // child
        sleep(5);
        exit(0);
    } else if (ret < 0) {
        // failed to create process
        print_error();
        exit(EXIT_FAILURE);
    } else {
        // parent process
        pid_t pid = 0;
        while (pid == 0) {
            if ((pid = waitpid(pid, NULL, WNOHANG)) == -1) {
                // failed to get child process state
                print_error();
            } 
        }
    }
}

int main(int argc, char *argv[]) {

    // run command loop
    while (1) {

        // batch or interactive mode?
        if (is_interactive_mode(argc)) {
            print_ps1();
        }

        // read line on input
        char *line = wish_read_line();

        // parse line into tokens
        wish_tokenize_line(line);

        // execute each command in line as its own process (except built-ins)
        wish_execute();
    }
}