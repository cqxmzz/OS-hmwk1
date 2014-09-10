#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#define MAX_NUMBER_OF_ARGS 500

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
                        input_string[0] = '\0';
                }
                else{
                        input_string = realloc(input_string, input_length + 1);
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
                        char *arg = (char *)malloc(i - former_space);
                        args[*number_of_args] = arg;
                        (*number_of_args)++;
                        strncpy (arg, input_string + former_space + 1, i - former_space - 1);
                        arg[i - former_space - 1] = '\0';
                        former_space = i;
                }
                if (*number_of_args == MAX_NUMBER_OF_ARGS){
                        return -1;
                }
        }
        args[*number_of_args] = NULL;
        return 0;
}

int main(int argc, char **argv) 
{
        while (1){
                printf("$");
                char *input_string = NULL;
                input_string = read_line();
                /*
                 * exit case
                 */
                if (!strcmp(input_string, "exit")){
                        exit(EXIT_SUCCESS);
                }

                if (!strcmp(input_string, "")){
                        continue;
                }
                /*
                 * parsing
                 */
                int number_of_args;
                char *args[MAX_NUMBER_OF_ARGS];
                int parse_return = parse(input_string, MAX_NUMBER_OF_ARGS, args, &number_of_args);
                if (parse_return != 0){
                        fprintf(stderr, "error: %s\n", "Too many arguments");
                        continue;
                }
                /*
                 * execute
                 */
                int return_status = -1;
                int pid = fork();
                if (pid == 0){
                        return_status = execv(args[0], args);
                        if (return_status == -1){
                                fprintf(stderr, "error: %s\n", strerror(errno));
                        }
                        exit(EXIT_FAILURE);
                }else if (pid > 0){
                        wait(0);
                }else{
                        fprintf(stderr, "error: %s\n", strerror(errno));
                        continue; 
                }
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
