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
                if (fgets(temp_buffer, chunk, stdin) == NULL){
                        fprintf(stderr, "error: %s\n", "Get unexpected character");
                        exit(-1);
                }
                temp_length = strlen(temp_buffer);
                input_length += temp_length;
                if (input_string == NULL) {
                        input_string = malloc((input_length + 1) * sizeof(char));
                        if (input_string == NULL){
                                fprintf(stderr, "error: %s\n", strerror(errno));
                                return NULL;
                        }
                        input_string[0] = '\0';
                }
                else{
                        input_string = realloc(input_string, input_length + 1);
                        if (input_string == NULL){
                                fprintf(stderr, "error: %s\n", strerror(errno));
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
                if (input_string[i] == ' ' || input_string[i] == '\t' || input_string[i] == '|' || i == strlen(input_string)){
                        if (i == former_space + 1)
                                former_space++;
                        else{
                                char *arg = (char *)malloc((i - former_space) * sizeof(char));
                                if (arg == NULL){
                                        fprintf(stderr, "error: %s\n", strerror(errno));
                                        return -1;
                                }
                                args[*number_of_args] = arg;
                                (*number_of_args)++;
                                strncpy (arg, input_string + former_space + 1, i - former_space - 1);
                                arg[i - former_space - 1] = '\0';
                                former_space = i;
                        }
                        if (input_string[i] == '|'){
                                char *arg = (char *)malloc(2 * sizeof(char));
                                if (arg == NULL){
                                        fprintf(stderr, "error: %s\n", strerror(errno));
                                        return -1;
                                }
                                args[*number_of_args] = arg;
                                (*number_of_args)++;
                                strncpy (arg, input_string + i, 1);
                                arg[1] = '\0';
                                former_space = i;
                        }
                }
                if (*number_of_args == MAX_NUMBER_OF_ARGS - 1){
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
                printf("%s\n", paths[number_of_paths - 1]);
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
                        fprintf(stderr, "error: %s\n", strerror(errno));
                        return -1;
                }
        }
        if (number_of_paths != 0 && number_of_paths == path_capacity){
                char **temp_paths = (char **)malloc(2 * number_of_paths * sizeof (char *));
                if (paths == NULL){
                        fprintf(stderr, "error: %s\n", strerror(errno));
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
                fprintf(stderr, "error: %s\n", strerror(errno));
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

int execute(char *file_name, char **args, int pipes[][2], int program_count, int program_no)
{
        int pid = fork();
        if (pid == 0){
                int pipe_in = -1;
                int pipe_out = -1;
                int fd_in = -1;
                int fd_out = -1;
                if (program_no != 0){
                        pipe_in = pipes[program_no - 1][0];
                        close(STDIN_FILENO);
                        fd_in = dup2(pipe_in, STDIN_FILENO);
                }
                if (program_no != program_count - 1){
                        pipe_out = pipes[program_no][1];
                        close(STDOUT_FILENO);
                        fd_out = dup2(pipe_out, STDOUT_FILENO);
                }
                int i;
                for (i = 0; i < program_count - 1; ++i){
                        if (pipes[i][0] != pipe_in)
                                close(pipes[i][0]);
                        if (pipes[i][1] != pipe_out)
                                close(pipes[i][1]);
                }
                if (execv(file_name, args) == -1)
                {
                        if (pipe_in != -1) close(pipe_in);
                        if (pipe_out != -1) close(pipe_out);
                        if (fd_in != -1) close(fd_in);
                        if (fd_out != -1) close(fd_out);
                        fprintf(stderr, "error: %s\n", strerror(errno));
                        exit(-1);
                }else{
                        if (pipe_in != -1) close(pipe_in);
                        if (pipe_out != -1) close(pipe_out);
                        if (fd_in != -1) close(fd_in);
                        if (fd_out != -1) close(fd_out);
                        exit(0);
                }
        }else if (pid > 0){
                if (program_no < program_count - 1)
                        return 0;
                int i;
                for (i = 0; i < program_count - 1; ++i){
                        close(pipes[i][0]);
                        close(pipes[i][1]);
                }
                int child_status;
                int wait_return = waitpid(pid, &child_status, 0);
                if (wait_return > 0){
                        return WEXITSTATUS(child_status);
                }else{
                        fprintf(stderr, "error: %s\n", strerror(errno));
                        return -1;
                }
        }else{
                fprintf(stderr, "error: %s\n", strerror(errno));
                return -1; 
        }
}

int find_file_to_exec(char **args, int pipes[][2], int program_count, int program_no)
{
        char *file_name = args[0];
        if (access(file_name, X_OK) != -1){
                if (execute(file_name, args, pipes, program_count, program_no) == 0)
                        return 0;
                else
                        return -1;
        }else{
                int i;
                for (i = 0; i < number_of_paths; ++i){
                        char *path_and_file_name = malloc((strlen(paths[i]) + strlen(file_name) + 1) * sizeof(char));
                        if (path_and_file_name == NULL){
                                fprintf(stderr, "error: %s\n", strerror(errno));
                        }
                        path_and_file_name[0] = '\0';
                        strcat(path_and_file_name, paths[i]);
                        strcat(path_and_file_name, "/");
                        strcat(path_and_file_name, file_name);
                        if (access(path_and_file_name, X_OK) != -1){
                                file_name = path_and_file_name;
                                int execute_return = execute(file_name, args, pipes, program_count, program_no);
                                free(file_name);
                                return execute_return;
                        }
                }
        }
        errno = ENOENT;
        fprintf(stderr, "error: %s\n", strerror(errno));
        return -2;
}

int multi_find_file_to_exec(char **args, int number_of_args)
{
        int last_seperate = -1;
        int program_count = 0;
        int i;
        int pipes[500][2];
        int program_heads[500];
        for (i = 0; i < number_of_args + 1; ++i){
                if (i == number_of_args || !strcmp("|", args[i])){
                        free(args[i]);
                        args[i] = NULL;
                        program_heads[program_count] = last_seperate + 1;
                        last_seperate = i;
                        program_count++;
                }
        }
        for (i = 0; i < program_count - 1; ++i){
                pipe(pipes[i]);
                //error
        }
        for (i = 0; i < program_count; ++i){
                if (find_file_to_exec(args + program_heads[i], pipes, program_count, i) != 0)
                        return -1;
        }
        //clear pipes
        return 0;
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
                 * parsing
                 */
                int number_of_args;
                char *args[MAX_NUMBER_OF_ARGS];
                int parse_return = parse(input_string, MAX_NUMBER_OF_ARGS, args, &number_of_args);
                if (parse_return != 0){
                        continue;
                }
                /*
                 * empty line
                 */
                if (args[0] == NULL || !strcmp(args[0], ""))
                        continue;
                /*
                 * exit case
                 */
                if (!strcmp(args[0], "exit"))
                        exit(EXIT_SUCCESS);
                /*
                 * cd
                 */
                if (!strcmp(args[0], "cd")){
                        cd(args[1]);
                        continue;
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
                multi_find_file_to_exec(args, number_of_args);
                /*
                 * free memory
                 */
                int i;
                for (i = 0; i < number_of_args; ++i){
                        if (args[i] != NULL)
                                free(args[i]);
                        args[i] = NULL;
                }
                free(input_string);
                input_string = NULL;
        }
        return 0;
}
