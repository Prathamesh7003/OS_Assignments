#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_COMMAND_LENGTH 200
#define MAX_ARGUMENTS 20
#define MAX_PATH_LENGTH 200
#define MAX_VARIABLES 200

char* custom_prompt = NULL; 
char* custom_path = NULL;
char* custom_var[MAX_VARIABLES]; 
int custom_var_count = 0; 

//it sets the new promptname or to cwd
void custom_prompt_seting(const char* new_prompt)   
{
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
//it display the prompt name or cwd
void prompt_display()              
{
    if (custom_prompt != NULL) 
    {
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
//it sets the custom path 
void custom_path_seting(const char* new_path) 
{
    if (custom_path != NULL) {
        free(custom_path);
    }
    custom_path = strdup(new_path);
}
//it sets the variable to the given value in terminal
void custom_var_seting(const char* name, const char* value)
{
    for (int i = 0; i < custom_var_count; i++) 
    {
        char* var = custom_var[i];
        if (strncmp(var, name, strlen(name)) == 0 && var[strlen(name)] == '=') 
        {
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
//it gets the value assigned to the variable in terminal
char* custom_variable_get(const char* name) 
{
    for (int i = 0; i < custom_var_count; i++) {
        char* var = custom_var[i];
        if (strncmp(var, name, strlen(name)) == 0 && var[strlen(name)] == '=') 
        { return var + strlen(name) + 1;
        }
    }return NULL;
}
//it substitutes the variable with given/past value
void custom_substitute_variables(char** arguments) 
{
    for (int i = 0; arguments[i] != NULL; i++) {
        if (arguments[i][0] == '$') 
        {
            char* value = custom_variable_get(arguments[i] + 1);
            if (value != NULL) {
                free(arguments[i]);
                arguments[i] = strdup(value);
            }
        }
    }
}
//it dissects the command into arguments in terminal
void custom_dissect_command(char* command, char** arguments) 
{
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
//it redirects the output to file mentioned in the terminal
void redirection_output(char** arguments) 
{
    for (int i = 0; arguments[i] != NULL; i++) {
        if (strcmp(arguments[i], ">") == 0) {                 //output
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
//it redirects the input to file mentioned in the terminal
void redirection_input(char** arguments) 
{
    for (int i = 0; arguments[i] != NULL; i++) {
        if (strcmp(arguments[i], "<") == 0)                 //input
        {  
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
//it execuates the inbuilt command and also command given in the path
void implement_command(char** arguments) 
{
    if (arguments[0] == NULL) {
        return;
    }
    int stdout_save = dup(STDOUT_FILENO);
    int stdin_save = dup(STDIN_FILENO);

    redirection_input(arguments);
    redirection_output(arguments);

    if (strcmp(arguments[0], "echo") == 0 && arguments[1] != NULL && arguments[1][0] == '$') 
    {
        char* value = custom_variable_get(arguments[1] + 1);
        if (value != NULL) {
            printf("%s\n", value);
        } else {
            printf("\n");
        }
        return;
    }
    custom_substitute_variables(arguments);
    pid_t pid = fork();
    if (pid == 0) 
    {
        if (strcmp(arguments[0], "cd") == 0) 
        {
            if (arguments[1] != NULL) {
                if (chdir(arguments[1]) != 0) {
                    perror("chdir() error");
                }
            } else {
                fprintf(stderr, "cd: expected further argument\n");
            }
        } 
        else 
        {
            char* path_copy = strdup(custom_path);
            char* dir = strtok(path_copy, ":");
            while (dir != NULL) 
            {
                char execute[MAX_PATH_LENGTH];
                snprintf(execute, sizeof(execute), "%s/%s", dir, arguments[0]);
                if (access(execute, X_OK) == 0) 
                {
                    execv(execute, arguments);
                    perror("execv() error");
                    exit(EXIT_FAILURE);
                }
                dir = strtok(NULL, ":");
            }
            free(path_copy);
            fprintf(stderr, "Error: command is not found\n");
            exit(EXIT_FAILURE);
        }
    } 
    else if (pid > 0) 
    {
        int status;
        wait(&status);
    } 
    else 
    {
        perror("fork() error");
    }
    dup2(stdout_save, STDOUT_FILENO);
    dup2(stdin_save, STDIN_FILENO);
    close(stdout_save);
    close(stdin_save);
}
int main() 
{
    char custom_command[MAX_COMMAND_LENGTH];
    char* arguments[MAX_ARGUMENTS];

    custom_path = strdup("/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin:/snap/bin"); 
    custom_var_seting("PATH", "usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin:/snap/bin");

    while (1) 
    {
        prompt_display();

        if (fgets(custom_command, sizeof(custom_command), stdin) == NULL) {
            printf("\n");
            break;
        }
        custom_command[strcspn(custom_command, "\n")] = '\0';

        if (strcmp(custom_command, "exit") == 0) {
            break;
        }
        if (strlen(custom_command) >= MAX_COMMAND_LENGTH) {
            fprintf(stderr, "Error: command too long\n");
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
            custom_dissect_command(custom_command, arguments);
            implement_command(arguments);
        }
    }
    for (int i = 0; i < custom_var_count; i++) 
    {
        free(custom_var[i]);
    }
    free(custom_path);
    free(custom_prompt);
    return 0;
}
