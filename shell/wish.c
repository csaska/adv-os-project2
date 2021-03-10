#include "ctype.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"

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

// built-ins: cd, exit, and path
int wish_cd(int argc, char *args[]) {
    printf("We are in wish_cd");
    // check exactly 1 arg is passed to cd
    if (argc != 2) {
        print_error();
        return EXIT_FAILURE;
    }
    printf("Calling chdir with %s", args[1]);
    if (chdir(args[1]) == -1) {
        print_error();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int wish_exit(int argc, char *args[]) {
    // check exactly no args are passed to exit
    if (argc != 1) {
        print_error();
        return EXIT_FAILURE;
    }
    exit(0);
}

int wish_path(int argc, char *args[]) {
    if (argc == 1) {
        // TODO: clear path
    } else {
        // TODO: add each arg to $PATH
        // TODO: should we error check that they are all delimited by whitespace?
    }
    return 0;
}

char *builtin_options[] = {
  "cd",
  "exit",
  "path"
};

int num_of_builtins() {
  return sizeof(builtin_options) / sizeof(char *);
}

int (*builtin_func[]) (int argc, char **) = {
  &wish_cd,
  &wish_exit,
  &wish_path,
};

char* wish_read_line(FILE *input) {
    char *line = NULL;
    size_t buffsize = 0;
    if (getline(&line, &buffsize, input) == -1) {
        if (feof(input)) {
           if (input != stdin) {
                printf("we are in here 1");
                fclose(input); 
            }
            printf("we are in here 2");
            exit(EXIT_SUCCESS);
        } else {
            print_error();
            exit(EXIT_FAILURE);
        }
    }
    printf("we are here 3");
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
    int argc;
    char **args;
    char *outfile;
    struct Command *next;
};

#define ARG_BUFSIZE 64
#define ARG_DELIM " \t"

struct Command* create_command_from_string(char *trimmed_string) {
    // Split command into args
    int bufsize = ARG_BUFSIZE, position = 0;
    char **args = malloc(bufsize * sizeof(char*));
    char *token;

    if (args == NULL) {
        print_error();
        exit(EXIT_FAILURE);
    }

    token = strtok(trimmed_string, ARG_DELIM);
    while (token != NULL) {
        args[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += ARG_BUFSIZE;
            args = realloc(args, bufsize * sizeof(char*));
            if (!args) {
                print_error();
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, ARG_DELIM);
    }
    args[position] = NULL;

    // Create command object
    struct Command *node = (struct Command*)malloc(sizeof(struct Command)); 
    if (node == NULL) {
        print_error();
        exit(EXIT_FAILURE);
    }
    node->cmd = args[0];
    node->next = NULL;
    node->args = args;
    node->argc = position;



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
    if (result == NULL) {
        print_error();
        return "";
    }

    strcpy(result, s1);
    strcat(result, "/");
    strcat(result, s2);
    return result;
}

char *find_command_in_path(char *cmd, char *PATH) {
    char *path;
    const char delim[2] = ":";

    path = strtok(PATH, delim);
    while (path != NULL) {
        // test if command exists at directory from $PATH
        char* exe = concat(path, cmd);
        int result = access(exe, F_OK);
        if (result == 0) {
            return exe;
        }
        free(exe);

        // move on to the next path in $PATH
        path = strtok(NULL, delim);
    }
    return "";
}

void print_command(struct Command *curr_cmd) {
    printf("cmd=%s, argc=%d, ", curr_cmd->cmd, curr_cmd->argc);
    for (int i = 0; i < curr_cmd->argc; i++) {
        if (i > 0) {
            printf(", ");
        }
        printf("args_%d=%s", i, curr_cmd->args[i]);
    }
    printf("\n");
}

void wish_execute(struct Command *curr_cmd) {
    // run built-ins using syscalls
    for (int i = 0; i < num_of_builtins(); i++) {
        if (strcmp(curr_cmd->cmd, builtin_options[i]) == 0) {
            printf("Executing builtin %s\n", builtin_options[i]);
            print_command(curr_cmd);
            (*builtin_func[i])(curr_cmd->argc, curr_cmd->args);

            // TODO: free member variables of struct
            free(curr_cmd);
            return;
        }
    }

    // run commands as child process
    while (curr_cmd != NULL) {
        print_command(curr_cmd);

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
                execv(exe, curr_cmd->args);

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
        printf("line read start");
        char *line = wish_read_line(input);
        printf("line read end");

        // parse line into tokens
        struct Command *cmd_list = wish_tokenize_line(line);

        // execute each command in line as its own process (except built-ins)
        wish_execute(cmd_list);
    }
}
