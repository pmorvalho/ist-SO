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

int numchildren = 0, exit_flag=1;
list_t* proc_list = lst_new();
pthread_mutex_t latch;


void *monitor() {
  int status=0, childpid=0;
  time_t gandalf_time;

  while (1) {
      //lock
    if(numchildren > 0) {
      //unlock
      childpid = wait(&status);
      gandalf_time = time(NULL);

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
      //lock
      update_terminated_process(proc_list, childpid, status, gandalf_time);
      numchildren --;
      //unlock
    }

    else
      if (exit_flag==0) {
        //unlock
        break;
      }
      //unlock
      sleep(1);

  }
  return NULL;
}



int main (int argc, char** argv) {

  char *args[MAXARGS];

  char buffer[BUFFER_SIZE];

  pthread_t monitor;

  pthread_mutex_init(&latch, );

  if (pthread_create(&monitor, 0, monitor(), NULL) != 0) {
    printf("Erro na criacao da tarefa\n");
    exit(1);
  }
  else {
    printf("Criada a tarefa %d\n", monitor);
  }

  printf("Insert your commands:\n");

  while (1) {
    int numargs;

    numargs = readLineArguments(args, MAXARGS, buffer, BUFFER_SIZE);

    /* Verifica se: se chegou ao EOF (end of file) do stdin
       ou se chegou a ordem "exit". Em ambos os casos, termina
       ordeiramente esperando pela terminacao dos filhos */
    if (numargs < 0 || (numargs > 0 && (strcmp(args[0], EXIT_COMMAND) == 0))) {
      //lock
      exit_flag=0;
      //unlock
      pthread_join(monitor, NULL);
      lst_print(proc_list);
      exit(EXIT_SUCCESS);
    }

    /* Caso tenha havido argumentos e nao seja "exit", lancamos processo filho */
    else if (numargs > 0) {

      int pid = fork();
      //lock
      insert_new_process(proc_list, pid, 0, time(NULL));
      //unlock

      if (pid < 0) {
	      perror("Failed to create new process.");
	      exit(EXIT_FAILURE);
      }

      if (pid > 0) { 	  /* Codigo do processo pai */
        //lock
	      numchildren ++;
        //unlock
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
