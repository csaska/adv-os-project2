#include "ctype.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"


// built-ins: exit, cd, path

void print_ps1() {
    fprintf(stderr, "wish> ");
}

void print_error() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

void validate_argv(int argc, char *argv[]) {
    // if batch mode make sure batch file can be read
    if (argc == 2 && access(argv[1], R_OK) != 0) {
        print_error();
        exit(EXIT_FAILURE);
    }

    // only suppport zero or 1 positional arguments
    if (argc > 2) {
        print_error();
        exit(EXIT_FAILURE);
    }
}

FILE* open_stream(char *filename, char *mode) {
    FILE *fp = fopen(filename, mode);
    if (fp == NULL) {
        print_error();
        exit(EXIT_FAILURE);
    }
    return fp;
}

bool is_interactive_mode(int argc) {
    if (argc == 1) {
        return true;
    }
    return false;
}

char* wish_read_line(FILE *input) {
    char *line = NULL;
    size_t buffsize = 0;
    if (getline(&line, &buffsize, input) == -1) {
        if (feof(input)) {
           if (input != stdin) {
                fclose(input); 
            }
            exit(EXIT_SUCCESS);
        } else {
            print_error();
            exit(EXIT_FAILURE);
        }
    }
    return line;
}

// https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
char *trimwhitespace(char *str) {
    char *end;

    // Trim leading space
    while(isspace((unsigned char)*str)) str++;

    if(*str == 0)  // All spaces?
      return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator character
    end[1] = '\0';

    return str;
}


struct Command {
    char *cmd;
    char *outfile;
    struct Command *next;
};

struct Command* create_command_from_string(char *trimmed_string) {
    struct Command *node = (struct Command*)malloc(sizeof(struct Command)); 
    node->cmd = trimmed_string;
    node->next = NULL;
    if (node == NULL) {
        print_error();
        exit(EXIT_FAILURE);
    }

    // split on first white space and save to node->cmd

    // then search for >, save remaining string to outfile

    // check that outfile is exactly one string and has not '>' operators

    // break string into command, args, file redirection
    //  token = strtok(line, "&")
    //  while(token != NULL) {
    //      token = strtok(NULL, "&");
    //  }

    return node;
}

struct Command* wish_tokenize_line(char *line) {
    // Extract the first token
    char *token = strtok(line, "&");

    // loop through the string to extract all commands
    struct Command *head = NULL;
    struct Command *curr_node = NULL;
    while(token != NULL) {
        char *trimmed_token = trimwhitespace(token);
        if (strcmp(trimmed_token, "") != 0) {
            // Create command and add to linked list
            struct Command *node = create_command_from_string(trimmed_token);
            if (head == NULL) {
                head = node;
                curr_node = head;
            } else {
                curr_node->next = node;
                curr_node = node;
            }
        }
        // move on to next command
        token = strtok(NULL, "&");
    }
    return head;
}

char* concat(const char *s1, const char *s2) {
    char *result = malloc(strlen(s1) + strlen(s2) + 2);

    // TODO: check if malloc failed

    strcpy(result, s1);
    strcat(result, "/");
    strcat(result, s2);
    return result;
}

char *find_command_in_path(char *cmd, char *path) {
    char *token;
    const char delim[2] = ":";

    token = strtok(path, delim);
    while (token != NULL) {
        // test if command exists at directory from $PATH
        char* exe = concat(token, cmd);
        int result = access(exe, F_OK);
        if (result == 0) {
            return exe;
        }
        free(exe);

        // move on to the next path in $PATH
        token = strtok(NULL, delim);
    }
    return "";
}

void wish_execute(struct Command *curr_cmd) {
    while (curr_cmd != NULL) {
        // find cmd in $PATH
        char *path = strdup(getenv("PATH"));
        char *exe = find_command_in_path(curr_cmd->cmd, path);

        if (strlen(exe) == 0) {
            print_error();
        } else {
            // start process and execute commands
            int ret = fork();
            if (ret == 0) {
                // child
                char *args[] = { curr_cmd->cmd, NULL };
                execv(exe, args);

                // TODO: exit with error code of execv
                exit(0);
            } else if (ret < 0) {
                // failed to create process
                print_error();
            }

            // free space used for exe string
            free(exe);
        }

        // iterate to next command in list
        struct Command *temp = curr_cmd->next;
        free(curr_cmd);
        curr_cmd = temp;

        free(path);
    }

    // Wait for all child processes to finish
    while (wait(NULL) > 0);
}

int main(int argc, char *argv[]) {
    // ensure proper arguments passed
    validate_argv(argc, argv);

    FILE *input = argc == 2 ? open_stream(argv[1], "r") : stdin;

    // run command loop
    while (true) {

        // batch or interactive mode?
        if (is_interactive_mode(argc)) {
            print_ps1();
        }

        // read line on input
        char *line = wish_read_line(input);

        // parse line into tokens
        struct Command *cmd_list = wish_tokenize_line(line);

        // execute each command in line as its own process (except built-ins)
        wish_execute(cmd_list);
    }
}
