#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include "commandlinereader.h"

#define ARGNUM 7
void exit_results(int n_args);

int main() {
	int pid, argCnt=0;
	char * argVector[ARGNUM];

	while(1) {
		while (readLineArguments(argVector, ARGNUM) == 0);

		if (strcmp(argVector[0],"exit")==0) { break; }

		pid=fork();

		if (pid < 0) { perror("Erro no fork()\n"); }

		else if (pid == 0) {
      execv(argVector[0], argVector);
      perror("Erro no processo\n");
      exit(-1);
		}

		else {
			free(argVector[0]);
			argCnt++;
		}
	}

	exit_results(argCnt);

	return 0;
}

void exit_results(int n_args){
	int i;
	int *pids=NULL;
	int *status=NULL;

	pids = (int*) malloc(sizeof(int) * n_args);
	status = (int*) malloc(sizeof(int) * n_args);

	for (i=0; i<n_args; i++) {
		 pids[i]= wait(status + i);
	}

	for (i=0; i<n_args; i++) {
		if(WIFEXITED(status[i]))
			printf("Process %d terminated with status %d!\n", pids[i], WEXITSTATUS(status[i]));
	}

	free(pids);
	free(status);
}
