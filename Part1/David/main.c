#include "commandlinereader.h"
#include "defines.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#define N_ARGS 5
int runningProcesses = 0;

void newProcess(char * const *args);
void exitParShell();
void showPrompt();


int main() {
  char *args[N_ARGS];

  while(1) {
    //showPrompt(); //?
    if (readLineArguments(args, N_ARGS) <= 0) continue;
    if (strcmp(args[0],"exit") == 0) break;
    newProcess(args);
  }

  exitParShell();

  return 0;
}

/*
  Creates a new process and runs the program specified by args[0]
*/
void newProcess(char * const *args) {
  int pid = fork();
  if (pid == 0) {
    execv(args[0], args);
    fprintf(stderr, "Erro no carregamento do programa %s\n", args[0]);
    exit(-1);
  }
  else if (pid == -1) { //if fork failed
    perror("Erro na criação do processo-filho:\n");
  }
  else { //fork worked and we are in the parent process
    runningProcesses++;
  }
}

/*
  Gracefully exists parshell
*/
void exitParShell() {
  int i;
  int returnCode;
  int pid;
  int *returnCodes = NULL;
  int *pids = NULL;
  pids = malloc(sizeof(int) * runningProcesses);
  returnCodes = malloc(sizeof(int) * runningProcesses);
  TESTMEM(pids); //checks if malloc worked
  TESTMEM(returnCodes);

  //wait for all the child processes to finish
  for (i=0; i<runningProcesses; i++) {
    pids[i] = wait(returnCodes + i);
  }


  //output the returnCodes for all child processes
  while(runningProcesses--) {
    pid        = pids[runningProcesses];
    returnCode = returnCodes[runningProcesses];
    if (returnCode == 0)
      printf("Process %d terminated with success!\n", pid);
    else
      printf("Process %d terminated with error %d\n", pid, returnCode);
  }
  free(pids);
  free(returnCodes);
}

/*
  Outputs to stdout the current working directory
*/
void showPrompt() {
  char *pwd = getcwd(NULL,42);
  TESTMEM(pwd);
  printf("%s$ ", pwd);
  free(pwd);
}
