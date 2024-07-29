
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_COMMAND_LENGTH 200
#define MAX_ARGUMENTS 20
#define MAX_PATH_LENGTH 200
#define MAX_VARIABLES 200
#define MAX_HISTORY 10

char* custom_prompt = NULL; 
char* custom_path = NULL;
char* custom_var[MAX_VARIABLES]; 
int custom_var_count = 0; 
char history[MAX_HISTORY][MAX_COMMAND_LENGTH];
int history_index = 0;

pid_t child_pid = -1;

void sigint_handler(int sig) {
    fflush(stdout);
    if (child_pid > 0) {
        kill(child_pid, SIGINT);
    }
}

void add_to_history(const char* command) {
    strncpy(history[history_index], command, MAX_COMMAND_LENGTH);
    history_index = (history_index + 1) % MAX_HISTORY;
}

void print_history() {
    for (int i = 0; i < MAX_HISTORY; i++) {
        int index = (history_index + i) % MAX_HISTORY;
        if (strlen(history[index]) > 0) {
            printf("%s\n", history[index]);
        }
    }
}

void custom_prompt_seting(const char* new_prompt) {
    if (custom_prompt != NULL) {
        free(custom_prompt);
    }
    if (strcmp(new_prompt, "\\w$") == 0) {
        custom_prompt = NULL;
    } else {
        char* new_prompt_with_sign = malloc(strlen(new_prompt) + 2); 
        strcpy(new_prompt_with_sign, new_prompt);
        strcat(new_prompt_with_sign, ">");
        custom_prompt = new_prompt_with_sign;
    }
}

void prompt_display() {
    if (custom_prompt != NULL) {
        printf("%s ", custom_prompt);
    } else {
        char cwd[MAX_PATH_LENGTH];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s$ ", cwd);
        } else {
            perror("getcwd() error");
            return;
        }
    }
}

void custom_path_seting(const char* new_path) {
    if (custom_path != NULL) {
        free(custom_path);
    }
    custom_path = strdup(new_path);
}

void custom_var_seting(const char* name, const char* value) {
    for (int i = 0; i < custom_var_count; i++) {
        char* var = custom_var[i];
        if (strncmp(var, name, strlen(name)) == 0 && var[strlen(name)] == '=') {
            free(custom_var[i]);
            custom_var[i] = malloc(strlen(name) + strlen(value) + 2); 
            sprintf(custom_var[i], "%s=%s", name, value);
            return;
        }
    }
    char* var = malloc(strlen(name) + strlen(value) + 2); 
    sprintf(var, "%s=%s", name, value);
    custom_var[custom_var_count++] = var;
}

char* custom_variable_get(const char* name) {
    for (int i = 0; i < custom_var_count; i++) {
        char* var = custom_var[i];
        if (strncmp(var, name, strlen(name)) == 0 && var[strlen(name)] == '=') {
            return var + strlen(name) + 1;
        }
    }
    return NULL;
}

void custom_substitute_variables(char** arguments) {
    for (int i = 0; arguments[i] != NULL; i++) {
        if (arguments[i][0] == '$') {
            char* value = custom_variable_get(arguments[i] + 1);
            if (value != NULL) {
                free(arguments[i]);
                arguments[i] = strdup(value);
            }
        }
    }
}

void custom_dissect_command(char* command, char** arguments) {
    int i = 0;
    char* token = strtok(command, " ");
    while (token != NULL) {
        if (i >= MAX_ARGUMENTS) {
            fprintf(stderr, "Error: to many arguments\n");
            return;
        }
        arguments[i++] = token;
        token = strtok(NULL, " ");
    }
    arguments[i] = NULL;
}

void redirection_output(char** arguments) {
    for (int i = 0; arguments[i] != NULL; i++) {
        if (strcmp(arguments[i], ">") == 0) {
            char* filename = arguments[i + 1];

            arguments[i] = NULL; 
            arguments[i + 1] = NULL;

            int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC);
            if (fd == -1) {
                perror("open() file open error");
                exit(EXIT_FAILURE);
            }
            if (dup2(fd, STDOUT_FILENO) == -1) {
                perror("dup2() error");
                exit(EXIT_FAILURE);
            }
            close(fd);
            break;
        }
    }
}

void redirection_input(char** arguments) {
    for (int i = 0; arguments[i] != NULL; i++) {
        if (strcmp(arguments[i], "<") == 0) {  
            char* filename = arguments[i + 1];  
            arguments[i] = NULL; 
            arguments[i + 1] = NULL; 

            int fd = open(filename, O_RDONLY);
            if (fd == -1) {
                perror("open() file open error");
                exit(EXIT_FAILURE);
            }
            if (dup2(fd, STDIN_FILENO) == -1) {
                perror("dup2() error");
                exit(EXIT_FAILURE);
            }
            close(fd);
            break;
        }
    }
}
void implement_command(char** arguments) {
    if (arguments[0] == NULL) {
        return;
    }

    int pipefd[2];
    int pipe_index = -1;
    int last_fd = 0; // The last file descriptor, which is the read end of the previous pipe

    for (int i = 0; arguments[i] != NULL; i++) {
        if (strcmp(arguments[i], "|") == 0) {
            arguments[i] = NULL; // Terminate the command before the pipe
            pipe_index = i;

            if (pipe(pipefd) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }

            pid_t pid = fork();
            if (pid == 0) {
                // Child process: Close read end of the pipe and redirect stdout to write end of the pipe
                close(pipefd[0]); // Close the read end of the pipe
                if (last_fd != 0) {
                    // If this is not the first command, redirect stdin to the read end of the previous pipe
                    dup2(last_fd, STDIN_FILENO);
                }
                dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to the write end of the pipe
                execvp(arguments[0], arguments); // Execute the command before the pipe
                if (errno == ENOENT) {
                    fprintf(stderr, "%s: command not found\n", arguments[0]);
                } else {
                    perror("execvp");
                }
                exit(EXIT_FAILURE);
            } else if (pid < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            }

            close(pipefd[1]); // Close the write end of the pipe in the parent process
            if (last_fd != 0) {
                // If this is not the first command, close the read end of the previous pipe
                close(last_fd);
            }
            last_fd = pipefd[0]; // Save the read end of the current pipe

            arguments += pipe_index + 1; // Move the pointer to the next command
            i = -1; // Reset the index
        }
    }

    // Execute the last command in the pipeline
    pid_t pid = fork();
    if (pid == 0) {
        if (last_fd != 0) {
            // If this is not the first command, redirect stdin to the read end of the previous pipe
            dup2(last_fd, STDIN_FILENO);
        }
        execvp(arguments[0], arguments);
        if (errno == ENOENT) {
            fprintf(stderr, "%s: command not found\n", arguments[0]);
        } else {
            perror("execvp");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (last_fd != 0) {
        // If this is not the first command, close the read end of the previous pipe
        close(last_fd);
    }
    waitpid(pid, NULL, 0);
}
int main() {
    char custom_command[MAX_COMMAND_LENGTH];
    char* arguments[MAX_ARGUMENTS];

    custom_path = strdup("/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin:/snap/bin"); 
    custom_var_seting("PATH", "usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin:/snap/bin");

    // Register the SIGINT handler
    signal(SIGINT, sigint_handler);

    while (1) {
        prompt_display();

        if (fgets(custom_command, sizeof(custom_command), stdin) == NULL) {
            printf("\n");
            break;
        }
        custom_command[strcspn(custom_command, "\n")] = '\0';

        add_to_history(custom_command);

        if (strcmp(custom_command, "exit") == 0) {
            break;
        } else if (strcmp(custom_command, "history") == 0) {
            print_history();
            continue;
        } 
        if (strlen(custom_command) >= MAX_COMMAND_LENGTH) {
            fprintf(stderr, "Error: command too long\n");
            continue;
        }
        if (strncmp(custom_command, "cd", 2) == 0) {
            char* dir = strtok(custom_command + 3, " ");
            if (chdir(dir) != 0) {
            perror("cd failed");
            }
            continue;
        }
        if (strncmp(custom_command, "PS1=", 4) == 0) {
            custom_prompt_seting(custom_command + 4);
        } else if (strncmp(custom_command, "PATH=", 5) == 0) {
            custom_path_seting(custom_command + 5);
        } else if (custom_variable_get(custom_command) != NULL) {
            printf("%s\n", custom_variable_get(custom_command));
        } else if (strchr(custom_command, '=') != NULL) {
            char* name = strtok(custom_command, "=");
            char* value = strtok(NULL, "=");
            custom_var_seting(name, value);
        } else {
            child_pid = fork();
            if (child_pid == 0) {
                custom_dissect_command(custom_command, arguments);
                implement_command(arguments);
                exit(0);
            } else if (child_pid > 0) {
                waitpid(child_pid, NULL, 0);
                child_pid = -1;
            }
        }
    }
    for (int i = 0; i < custom_var_count; i++) {
        free(custom_var[i]);
    }
    free(custom_path);
    free(custom_prompt);
    return 0;
}
