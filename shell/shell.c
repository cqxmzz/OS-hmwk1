#include <stdio.h>
#include <string.h>

#define MAX_INPUT_LENGTH 1<<16

int main(int argc, char **argv) {
	while (1){
		printf("$");
		char inputString[MAX_INPUT_LENGTH];
		scanf("%s", inputString);
		/*
			exit case
		*/
		if (!strcmp(inputString, "exit")){
			break;
		}
	}
	return 0;
}
