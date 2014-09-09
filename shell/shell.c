#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(int argc, char **argv) 
{
        while (1){
                printf("$");
                char * input_string = NULL;
                input_string = read_line();
                /*
                 * exit case
                 */
                if (!strcmp(input_string, "exit")){
                        break;
                }
                free(input_string);
                input_string = NULL;
        }
        return 0;
}
