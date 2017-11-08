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
#define BUFFER_SIZE 100


int numchildren = 0, exit_flag=0;
list_t* proc_list;
pthread_mutex_t latch;


void *monitor_cicle() {
  int status=0, childpid=0;
  time_t end_time;

  while (1) {
    pthread_mutex_lock(&latch);
    if(numchildren > 0) {    /*verifica se ha filhos criados*/
      pthread_mutex_unlock(&latch);
      childpid = wait(&status);
      end_time = time(NULL);   /*guarda o tempo de fim de vida do filho*/

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
      update_terminated_process(proc_list, childpid, status, end_time); /* faz update na lista de filhos, adicionando-o */
      numchildren --;
      pthread_mutex_unlock(&latch);
    }

    else {
      if (exit_flag==1) { /* verifica de a flag do exit ja se encontra activa*/
        pthread_mutex_unlock(&latch);
        break;
      }
      pthread_mutex_unlock(&latch);
      sleep(1);  /*ao nao encontrar filhos "adormece" por 1 segundo*/
      }

  }
  pthread_exit(EXIT_SUCCESS);
}



int main (int argc, char** argv) {

  char *args[MAXARGS];

  char buffer[BUFFER_SIZE];

  pthread_t monitor;

  time_t start_time;

  int pid;


  pthread_mutex_init(&latch, NULL); /*cria o mutex latch*/

  proc_list = lst_new(); /*cria lista de filhos*/


  if (pthread_create(&monitor, 0, monitor_cicle, NULL) != 0) { /*verifica se a tarefa monitor foi criada*/
    printf("Error with pthread_create!\n");
    exit(EXIT_FAILURE);
  }


  printf("Insert your commands:\n");
  while (1) {
    int numargs;


    numargs = readLineArguments(args, MAXARGS, buffer, BUFFER_SIZE);

    /* Verifica se: se chegou ao EOF (end of file) do stdin
       ou se chegou a ordem "exit". Em ambos os casos, termina
       ordeiramente esperando pela terminacao dos filhos */

    if (numargs <= 0)
	   continue;
    if (strcmp(args[0], EXIT_COMMAND) == 0) { /*verifica se foi introduzido o comando 'exit'*/
      pthread_mutex_lock(&latch);
      exit_flag=1;   				/* altera a falg do exit */
      pthread_mutex_unlock(&latch);
      pthread_join(monitor, NULL);		/* espera que a tarefa monitor acabe*/
      lst_print(proc_list);			/* faz print da lista de filhos*/
      lst_destroy(proc_list);			/* destroi a lista*/
      pthread_mutex_destroy(&latch);		/* destroi o mutex*/
      exit(EXIT_SUCCESS);
    }

    /* Caso tenha havido argumentos e nao seja "exit", lancamos processo filho */

      pid = fork(); /*cria um filho*/
      start_time=time(NULL); /*guarda o tempo de inicio da vida do filho*/

      if (pid < 0) {
	      perror("Failed to create new process.\n");
	      exit(EXIT_FAILURE);
      }

      if (pid > 0) { 	  /* Codigo do processo pai */
        pthread_mutex_lock(&latch);
        insert_new_process(proc_list, pid, 0, start_time);
	      numchildren ++;
        pthread_mutex_unlock(&latch);
      }

      else { /* Codigo do processo filho */
	      if (execv(args[0], args) < 0) {
	        perror("Could not run child program. Child will exit.\n");
	        exit(EXIT_FAILURE);
	      }
      }



  }

}
