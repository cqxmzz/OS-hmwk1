#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>

#define MAX_NUMBER_OF_ARGS 500

int number_of_paths = 0;
char **paths = NULL;
int path_capacity = 0;

char *read_line()
{
        char *input_string = NULL;
        int chunk = 4;
        char temp_buffer[chunk];
        int input_length = 0;
        int temp_length = 0;
        do {
                fgets(temp_buffer, chunk, stdin);
                temp_length = strlen(temp_buffer);
                input_length += temp_length;
                if (input_string == NULL) {
                        input_string = malloc((input_length + 1) * sizeof(char));
                        if (input_string == NULL){
                                fprintf(stderr, "error: %s\n", "Cannot require space to store the command, perhaps too long?");
                                return NULL;
                        }
                        input_string[0] = '\0';
                }
                else{
                        input_string = realloc(input_string, input_length + 1);
                        if (input_string == NULL){
                                fprintf(stderr, "error: %s\n", "Cannot require space to store the command, perhaps too long?");
                                return NULL;
                        }
                }
                strcat(input_string, temp_buffer);
        } while (temp_length == chunk - 1 && temp_buffer[chunk - 2] != '\n');
        input_string[input_length - 1] = '\0';
        return input_string;
}

int parse(char *input_string, int max_number_of_args, char *args[], int *number_of_args)
{
        int former_space = -1;
        *number_of_args = 0;
        int i;
        for (i = 0; i < strlen(input_string) + 1; ++i){
                if (input_string[i] == ' ' || i == strlen(input_string)){
                        char *arg = (char *)malloc((i - former_space) * sizeof(char));
                        if (arg == NULL){
                                fprintf(stderr, "error: %s\n", "Cannot require space to store the argument, perhaps too long?");
                                return -1;
                        }
                        args[*number_of_args] = arg;
                        (*number_of_args)++;
                        strncpy (arg, input_string + former_space + 1, i - former_space - 1);
                        arg[i - former_space - 1] = '\0';
                        former_space = i;
                }
                if (*number_of_args == MAX_NUMBER_OF_ARGS){
                        fprintf(stderr, "error: %s\n", "Too many arguments");
                        return -1;
                }
        }
        args[*number_of_args] = NULL;
        return 0;
}

int cd(char * path)
{
        if (path == NULL)
                return -1;
        if (chdir(path) == 0)
                return 0;
        else{
                fprintf(stderr, "error: %s\n", strerror(errno));                                 
                return -1;
        }
}

int show_paths()
{
        int i;
        for (i = 0; i < number_of_paths - 1; ++i)
                printf("%s:", paths[i]);
        if (number_of_paths > 0)
                printf("%s", paths[number_of_paths - 1]);
        return 0;
}

int add_path(char * path)
{
        if (path == NULL){
                fprintf(stderr, "error: %s\n", "Path not exist");
                return -1;
        }
        if (path == NULL){
                fprintf(stderr, "error: %s\n", "Bad path");
                return -1;
        }
        int i;
        for (i = 0; i < number_of_paths; ++i){
                if (!strcmp(path, paths[i])){
                        fprintf(stderr, "error: %s\n", "Path existed");
                        return -1;
                }
        }
        if (paths == NULL){
                paths = (char **)malloc(sizeof(char *));
                path_capacity = 1;
                if (paths == NULL){
                        fprintf(stderr, "error: %s\n", "Cannot require space to store the path, perhaps too many paths?");
                        return -1;
                }
        }
        if (number_of_paths != 0 && number_of_paths == path_capacity){
                char **temp_paths = (char **)malloc(2 * number_of_paths * sizeof (char *));
                if (paths == NULL){
                        fprintf(stderr, "error: %s\n", "Cannot require space to store the path, perhaps too many paths?");
                        return -1;
                }
                int i;
                for (i = 0; i < number_of_paths; ++i)
                        temp_paths[i] = paths[i];
                for (i = number_of_paths; i < 2 * number_of_paths; ++i)
                        temp_paths[i] = NULL;
                free(paths);
                paths = temp_paths;
                path_capacity *= 2;
        }
        paths[number_of_paths] = (char *)malloc(sizeof(char) * strlen(path));
        if (paths[number_of_paths] == NULL){
                fprintf(stderr, "error: %s\n", "Cannot require space to store the path, perhaps too many paths?");
                return -1;
        }
        strcpy(paths[number_of_paths], path);
        number_of_paths++;
        return 0;
}

int delete_path(char * path)
{
        if (path == NULL){
                fprintf(stderr, "error: %s\n", "Bad path");
                return -1;
        }
        int path_deleted = 0;
        int i;
        for (i = 0; i < number_of_paths; ++i)
        {
                if (!strcmp(path, paths[i]))
                {
                        int j;
                        char *temp = paths[i];
                        for (j = i; j < number_of_paths - 1; ++j){
                                paths[j] = paths[j+1];
                        }
                        number_of_paths--;
                        free(temp);
                        temp = NULL;
                        path_deleted = 1;
                        i--;
                }
        }
        if (path_deleted == 0){
                fprintf(stderr, "error: %s\n", "Path not found");
                return -1;
        }
        return 0;
}

int execute(char **args)
{
        int pid = fork();
        if (pid == 0){
                if (execv(args[0], args, "/bin/") == -1)
                {
                        fprintf(stderr, "error: %s\n", strerror(errno));
                        return -1;
                }
                else
                        return 0;
        }else if (pid > 0){
                wait(0);
                return 0;
        }else{
                fprintf(stderr, "error: %s\n", strerror(errno));
                return -1; 
        }
}

int main(int argc, char **argv) 
{
        while (1){
                printf("$");
                char *input_string = NULL;
                input_string = read_line();
                if (input_string == NULL)
                        continue;
                /*
                 * exit case
                 */
                if (!strcmp(input_string, "exit"))
                        exit(EXIT_SUCCESS);
                /*
                 * empty line
                 */
                if (!strcmp(input_string, ""))
                        continue;
                /*
                 * parsing
                 */
                int number_of_args;
                char *args[MAX_NUMBER_OF_ARGS];
                int parse_return = parse(input_string, MAX_NUMBER_OF_ARGS, args, &number_of_args);
                if (parse_return != 0){
                        continue;
                }
                /*
                 * cd
                 */
                if (!strcmp(args[0], "cd")){
                        cd(args[1]);
                }
                /*
                 * paths
                 */
                if (!strcmp(args[0], "path")){
                        if (args[1] == NULL){
                                show_paths();
                        }else if (!strcmp(args[1], "+"))
                        {
                                add_path(args[2]);
                        }else if (!strcmp(args[1], "-")){
                                delete_path(args[2]);
                        }else{
                                fprintf(stderr, "error: %s\n", "Path command error");
                        }
                        continue;
                }
                /*
                 * execute
                 */
                execute(args);
                /*
                 * free memory
                 */
                int i;
                for (i = 0; i < number_of_args; ++i){
                        free(args[i]);
                        args[i] = NULL;
                }
                free(input_string);
                input_string = NULL;
        }
        return 0;
}
