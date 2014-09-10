#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        for (int i = 0; i < strlen(input_string) + 1; ++i){
                if (input_string[i] == ' ' || i == strlen(input_string)){
                        char *arg = (char *)malloc(i - former_space);
                        args[*number_of_args] = arg;
                        (*number_of_args)++;
                        strncpy (arg, input_string + former_space + 1, i - former_space - 1);
                        arg[i - former_space - 1] = '\0';
                        former_space = i;
                }
                if (*number_of_args > MAX_NUMBER_OF_ARGS){
                        return -1;
                }
        }
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
                        break;
                }

                int number_of_args;
                char *args[MAX_NUMBER_OF_ARGS];
                parse(input_string, MAX_NUMBER_OF_ARGS, args, &number_of_args);

                for (int i = 0; i < number_of_args; ++i){
                    printf("%s\n", args[i]);
                }

                for (int i = 0; i < number_of_args; ++i){
                            free(args[i]);
                }
                free(input_string);
                input_string = NULL;
        }
        return 0;
}
