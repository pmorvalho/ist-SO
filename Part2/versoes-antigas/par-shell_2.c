/*
// Par-shell - exercicio 1, version 1
// Sistemas Operativos, DEI/IST/ULisboa 2015-16
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "commandlinereader.h"
#include "list.h"

#define EXIT_COMMAND "exit"
#define MAXARGS 7 /* Comando + 5 argumentos opcionais + espaco para NULL */
#define STATUS_INFO_LINE_SIZE 50
#define BUFFER_SIZE 100

int numchildren = 0;
list_t* proc_list = lst_new();


void monitor() {
  int status, childpid;

  while (1) {

    if(numchildren > 0) {
      childpid = wait(&status);
      update_terminated_process(proc_list, childpid, status, time(NULL));


      if (childpid < 0) {
        if (errno == EINTR) {
          /* Este codigo de erro significa que chegou signal que interrompeu a espera
          pela terminacao de filho; logo voltamos a esperar */
          continue;
        }
        else {
          perror("Error waiting for child.");
          exit (EXIT_FAILURE);
        }
      }

      numchildren --;
    }

    else
      sleep(1);

  }
  return NULL;
}



int main (int argc, char** argv) {

  char *args[MAXARGS];

  char buffer[BUFFER_SIZE];

  pthread_t monitor;

  printf("Insert your commands:\n");

  while (1) {
    int numargs;

    numargs = readLineArguments(args, MAXARGS, buffer, BUFFER_SIZE);

    /* Verifica se: se chegou ao EOF (end of file) do stdin
       ou se chegou a ordem "exit". Em ambos os casos, termina
       ordeiramente esperando pela terminacao dos filhos */
    if (numargs < 0 || (numargs > 0 && (strcmp(args[0], EXIT_COMMAND) == 0))) {

      if (pthread_create(&monitor, 0, monitor(), NULL) != 0) {
        printf("Erro na criacao da tarefa\n");
        exit(1);
      }
      else {
        printf("Criada a tarefa %d\n", monitor);
      }

      exit(EXIT_SUCCESS);
    }

    /* Caso tenha havido argumentos e nao seja "exit", lancamos processo filho */
    else if (numargs > 0) {

      int pid = fork();
      insert_new_process(proc_list, pid, 0, time(NULL));


      if (pid < 0) {
	      perror("Failed to create new process.");
	      exit(EXIT_FAILURE);
      }

      if (pid > 0) { 	  /* Codigo do processo pai */
	      numchildren ++;
	      continue;
      }

      else { /* Codigo do processo filho */
	      if (execv(args[0], args) < 0) {
	        perror("Could not run child program. Child will exit.");
	        exit(EXIT_FAILURE);
	      }
      }

    }

  }

}
