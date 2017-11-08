#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include "commandlinereader.h"

#define ARGNUM 7

/*
SHELL PARALELA - EXERCICIO 1
Grupo 40:
81151 - Pedro Orvalho
81365 - Ana Leitao
81647 - Manuel Galamba
*/

void exit_results(int n_args);

int main(int argc, char const *argv[]) {
	int pid, procCnt=0;
	char * argVector[ARGNUM];
	int num_max=atoi(argv[1]);

	while(1) {
		while (readLineArguments(argVector, ARGNUM) == 0);

		if ((strcmp(argVector[0],"exit")==0) || (num_max <= procCnt)) { break; }

		pid=fork();

		if (pid < 0) { perror("fork() error\n"); }

		else if (pid == 0) {
      		execv(argVector[0], argVector);
      		perror("execv() error\n");
      		exit(EXIT_FAILURE);
		}

		else { procCnt++; }

		if (num_max == procCnt) { break; }
	}

	exit_results(procCnt);

	return 0;
}

void exit_results(int n_proc){
	int i;
	int *pids=NULL;
	int *status=NULL;

	pids = (int*) malloc(sizeof(int) * n_proc);
	status = (int*) malloc(sizeof(int) * n_proc);

	for (i=0; i<n_proc; i++) {
		 pids[i]= wait(status + i);
		 if (pids[i] == -1)
		 	perror("wait() error\n");
	}

	for (i=0; i<n_proc; i++) {
		if(WIFEXITED(status[i]))
			printf("Process %d exited with status %d!\n", pids[i], WEXITSTATUS(status[i]));
		else
			printf("Process %d terminated without calling exit\n", pids[i]);
	}

	free(pids);
	free(status);
}
