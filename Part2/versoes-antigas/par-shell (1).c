/*
SHELL PARALELA - EXERCICIO 2
Grupo 40:
81151 - Pedro Orvalho
81365 - Ana Leitao
81647 - Manuel Galamba
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

int numchildren = 0, exit_flag=0;
list_t* proc_list;
pthread_mutex_t latch;


void *monitora() {
  int status=0, childpid=0;
  time_t end_time;

  while (1) {
    pthread_mutex_lock(&latch);
    if(numchildren > 0) {
      pthread_mutex_unlock(&latch);
      childpid = wait(&status);
      end_time = time(NULL);

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
      pthread_mutex_lock(&latch);
      update_terminated_process(proc_list, childpid, status, end_time);
      numchildren --;
      pthread_mutex_unlock(&latch);
    }

    else
      if (exit_flag==1) {
        pthread_mutex_unlock(&latch);
        break;
      }
      pthread_mutex_unlock(&latch);
      sleep(1);

  }
  return NULL;
}



int main (int argc, char** argv) {

  char *args[MAXARGS];

  char buffer[BUFFER_SIZE];

  pthread_t monitor;

  pthread_mutex_init(&latch, NULL);

  time_t start_time;

  proc_list = lst_new();

  if (pthread_create(&monitor, 0, monitora, NULL) != 0) {
    printf("Erro na criacao da tarefa\n");
    exit(1);
  }
  else {
    printf("Criada a tarefa monitora!\n");
  }

  printf("Insert your commands:\n");

  while (1) {
    int numargs;

    numargs = readLineArguments(args, MAXARGS, buffer, BUFFER_SIZE);

    /* Verifica se: se chegou ao EOF (end of file) do stdin
       ou se chegou a ordem "exit". Em ambos os casos, termina
       ordeiramente esperando pela terminacao dos filhos */
    if (numargs < 0 || (numargs > 0 && (strcmp(args[0], EXIT_COMMAND) == 0))) {
      pthread_mutex_lock(&latch);
      exit_flag=1;
      pthread_mutex_unlock(&latch);
      pthread_join(monitor, NULL);
      lst_print(proc_list);
      lst_destroy(proc_list);
      pthread_mutex_destroy(&latch);
      exit(EXIT_SUCCESS);
    }

    /* Caso tenha havido argumentos e nao seja "exit", lancamos processo filho */
    else if (numargs > 0) {

      int pid = fork();
      start_time=time(NULL);

      if (pid < 0) {
	      perror("Failed to create new process.");
	      exit(EXIT_FAILURE);
      }

      if (pid > 0) { 	  /* Codigo do processo pai */
        pthread_mutex_lock(&latch);
        insert_new_process(proc_list, pid, 0, start_time);
	      numchildren ++;
        pthread_mutex_unlock(&latch);
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
