#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

int number_of_paths = 0;
char **paths = NULL;
char **args = NULL;
int path_capacity = 0;

int report_error(int return_value, char *error_message)
{
	if (error_message == NULL)
		fprintf(stderr, "error: %s\n", strerror(errno));
	else
		fprintf(stderr, "error: %s\n", error_message);
	return return_value;
}

char *read_line()
{
	char *input_string = NULL;
	int chunk = 4;
	char temp_buffer[chunk];
	int input_length = 0;
	int temp_length = 0;

	do {
		if (fgets(temp_buffer, chunk, stdin) == NULL)
			exit(-1);
		temp_length = strlen(temp_buffer);
		input_length += temp_length;
		if (input_string == NULL) {
			input_string = malloc((input_length + 1)
				* sizeof(char));
			if (input_string == NULL) {
				report_error(-1, NULL);
				return NULL;
			}
			input_string[0] = '\0';
		} else {
			input_string = realloc(input_string, (input_length + 1)
				* sizeof(char));
			if (input_string == NULL) {
				report_error(-1, NULL);
				return NULL;
			}
		}
		strcat(input_string, temp_buffer);
	} while (temp_length == chunk - 1 && temp_buffer[chunk - 2] != '\n');
	input_string[input_length - 1] = '\0';
	return input_string;
}

int parse(char *input_string, int *number_of_args)
{
	int former_space = -1;
	*number_of_args = 0;
	int i;
	char *arg;

	if (args == NULL)
		args = malloc(sizeof(char *));
	for (i = 0; i < strlen(input_string) + 1; ++i) {
		if (input_string[i] == ' ' || input_string[i] == '\t' ||
			input_string[i] == '|' || i == strlen(input_string)) {
			if (i == former_space + 1)
				former_space++;
			else {
				arg = (char *)malloc((i - former_space)
					* sizeof(char));
				if (arg == NULL)
					return report_error(-1, NULL);
				args = realloc(args, (*number_of_args + 1)
					* sizeof(char *));
				if (args == NULL)
					return report_error(-1, NULL);
				args[*number_of_args] = arg;
				(*number_of_args)++;
				strncpy(arg, input_string + former_space + 1,
					i - former_space - 1);
				arg[i - former_space - 1] = '\0';
				former_space = i;
			}
			if (input_string[i] == '|') {
				arg = (char *)malloc(2 * sizeof(char));
				if (arg == NULL)
					return report_error(-1, NULL);
				args = realloc(args, (*number_of_args + 1)
					* sizeof(char *));
				if (args == NULL)
					return report_error(-1, NULL);
				args[*number_of_args] = arg;
				(*number_of_args)++;
				arg[0] = '|';
				arg[1] = '\0';
				former_space = i;
			}
		}
	}
	args = realloc(args, (*number_of_args + 1) * sizeof(char *));
	if (args == NULL)
		return report_error(-1, NULL);
	args[*number_of_args] = NULL;
	return 0;
}

int cd(char *path)
{
	if (path == NULL)
		return -1;
	if (chdir(path) == 0)
		return 0;
	return report_error(-1, NULL);
}

int show_paths(void)
{
	int i;

	for (i = 0; i < number_of_paths - 1; ++i)
		printf("%s:", paths[i]);
	if (number_of_paths > 0)
		printf("%s\n", paths[number_of_paths - 1]);
	return 0;
}

int add_path(char *path)
{
	struct stat s;
	int i;

	if (path == NULL)
		return report_error(-1, "Bad path");
	if (stat(path, &s) == -1 || !S_ISDIR(s.st_mode))
		return report_error(-1, NULL);
	for (i = 0; i < number_of_paths; ++i) {
		if (!strcmp(path, paths[i]))
			return report_error(-1, "Path existed");
	}
	if (paths == NULL) {
		paths = (char **)malloc(sizeof(char *));
		path_capacity = 1;
		if (paths == NULL)
			report_error(-1, NULL);
	}
	if (number_of_paths != 0 && number_of_paths == path_capacity) {
		paths = realloc(paths, 2 * number_of_paths * sizeof(char *));
		if (paths == NULL)
			report_error(-1, NULL);
		path_capacity *= 2;
	}
	paths[number_of_paths] = (char *)malloc(sizeof(char) * strlen(path));
	if (paths[number_of_paths] == NULL)
		report_error(-1, NULL);
	strcpy(paths[number_of_paths], path);
	number_of_paths++;
	return 0;
}

int delete_path(char *path)
{
	int i;
	int path_deleted = 0;
	int j;
	char *temp;

	if (path == NULL)
		report_error(-1, "Bad path");
	for (i = 0; i < number_of_paths; ++i) {
		if (!strcmp(path, paths[i])) {
			temp = paths[i];
			for (j = i; j < number_of_paths - 1; ++j)
				paths[j] = paths[j+1];
			number_of_paths--;
			free(temp);
			temp = NULL;
			path_deleted = 1;
			i--;
		}
	}
	if (path_deleted == 0)
		report_error(-1, "Path not found");
	return 0;
}

int execute(char *file_name, char **args_in, int pipes[][2], int program_count,
	int program_no)
{
	int pid = fork();

	/*
	 * Child process
	 */
	if (pid == 0) {
		int pipe_in = -1;
		int pipe_out = -1;
		int fd_in = -1;
		int fd_out = -1;
		int i;
		int execute_return;
		/*
		 * Dup pipes and close pipes
		 */
		if (program_no != 0) {
			pipe_in = pipes[program_no - 1][0];
			close(STDIN_FILENO);
			fd_in = dup2(pipe_in, STDIN_FILENO);
			if (fd_in == -1)
				report_error(-1, NULL);
		}
		if (program_no != program_count - 1) {
			pipe_out = pipes[program_no][1];
			close(STDOUT_FILENO);
			fd_out = dup2(pipe_out, STDOUT_FILENO);
			if (fd_out == -1)
				report_error(-1, NULL);
		}
		for (i = 0; i < program_count - 1; ++i) {
			if (pipes[i][0] != pipe_in && pipes[i][0] != -1)
				if (close(pipes[i][0]) == -1)
					report_error(-1, NULL);
			if (pipes[i][1] != pipe_out && pipes[i][1] != -1)
				if (close(pipes[i][1]) == -1)
					report_error(-1, NULL);
		}
		/*
		 * Execute and close pipe if fail
		 */
		execute_return = execv(file_name, args_in);
		if (pipe_in != -1)
			if (close(pipe_in) == -1)
				report_error(-1, NULL);
		if (pipe_out != -1)
			if (close(pipe_out) == -1)
				report_error(-1, NULL);
		if (fd_in != -1)
			if (close(fd_in) == -1)
				report_error(-1, NULL);
		if (fd_out != -1)
			if (close(fd_out) == -1)
				report_error(-1, NULL);
		if (execute_return == -1) {
			report_error(-1, NULL);
			exit(-1);
		}
		exit(0);
	} else if (pid > 0) {
		/*
		 * Parent process close pipes don't use
		 */
		int child_status;
		int wait_return;

		if (program_no != 0) {
			close(pipes[program_no - 1][0]);
			pipes[program_no - 1][0] = -1;
		}
		if (program_no != program_count - 1) {
			close(pipes[program_no][1]);
			pipes[program_no][1] = -1;
		}
		wait_return = waitpid(pid, &child_status, 0);
		if (wait_return > 0)
			return WEXITSTATUS(child_status);
		report_error(-1, NULL);
	}
	return report_error(-1, NULL);
}

int find_file_to_exec(char **args_in, int pipes[][2], int program_count,
	int program_no)
{
	char *file_name = args_in[0];
	int i;
	int execute_return;
	char *path_and_file_name;

	if (access(file_name, X_OK) != -1) {
		if (execute(file_name, args_in, pipes, program_count,
			program_no) == 0)
			return 0;
		return -1;
	}
	for (i = 0; i < number_of_paths; ++i) {
		path_and_file_name = malloc((strlen(paths[i])
			+ strlen(file_name) + 1) * sizeof(char));
		if (path_and_file_name == NULL)
			report_error(-1, NULL);
		path_and_file_name[0] = '\0';
		strcat(path_and_file_name, paths[i]);
		strcat(path_and_file_name, "/");
		strcat(path_and_file_name, file_name);
		if (access(path_and_file_name, X_OK) != -1) {
			file_name = path_and_file_name;
			execute_return = execute(file_name, args_in, pipes,
				program_count, program_no);
			free(file_name);
			return execute_return;
		}
	}
	errno = ENOENT;
	return report_error(-1, NULL);
}

int multi_find_file_to_exec(int number_of_args)
{
	int last_seperate = -1;
	int program_count = 0;
	int i;
	int pipes[number_of_args][2];
	int program_heads[number_of_args];

	for (i = 0; i < number_of_args + 1; ++i) {
		if (i == number_of_args || !strcmp("|", args[i])) {
			free(args[i]);
			args[i] = NULL;
			if (i == last_seperate + 1)
				return report_error(-1, "Bad command");
			program_heads[program_count] = last_seperate + 1;
			last_seperate = i;
			program_count++;
		}
	}
	for (i = 0; i < program_count - 1; ++i) {
		if (pipe(pipes[i]) == -1)
			return report_error(-1, NULL);
	}
	for (i = 0; i < program_count; ++i) {
		if (find_file_to_exec(args + program_heads[i], pipes,
			program_count, i) != 0)
			return -1;
	}
	for (i = 0; i < program_count - 1; ++i) {
		close(pipes[i][0]);
		close(pipes[i][1]);
	}
	return 0;
}

void free_memory(char *input_string, int number_of_args)
{
	int i;

	for (i = 0; i < number_of_args; ++i) {
		if (args[i] != NULL)
			free(args[i]);
		args[i] = NULL;
	}
	free(input_string);
	input_string = NULL;
}

int main(int argc, char **argv)
{
	while (1) {
		char *input_string = NULL;
		int number_of_args;
		int parse_return;

		printf("$");
		input_string = read_line();
		if (input_string == NULL)
			continue;
		/*
		 * parsing
		 */
		parse_return = parse(input_string, &number_of_args);
		if (parse_return != 0) {
			free_memory(input_string, number_of_args);
			continue;
		}
		/*
		 * empty line
		 */
		if (args[0] == NULL || !strcmp(args[0], "")) {
			free_memory(input_string, number_of_args);
			continue;
		}
		/*
		 * exit case
		 */
		if (!strcmp(args[0], "exit")) {
			free_memory(input_string, number_of_args);
			break;
		}
		/*
		 * cd
		 */
		if (!strcmp(args[0], "cd")) {
			cd(args[1]);
			free_memory(input_string, number_of_args);
			continue;
		}
		/*
		 * paths
		 */
		if (!strcmp(args[0], "path")) {
			if (args[1] == NULL)
				show_paths();
			else if (!strcmp(args[1], "+"))
				add_path(args[2]);
			else if (!strcmp(args[1], "-"))
				delete_path(args[2]);
			else
				report_error(-1, "Path command error");
			free_memory(input_string, number_of_args);
			continue;
		}
		/*
		 * execute
		 */
		multi_find_file_to_exec(number_of_args);
		free_memory(input_string, number_of_args);
	}
	return 0;
}
