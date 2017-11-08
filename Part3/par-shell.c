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
#include <semaphore.h>
#include "commandlinereader.h"
#include "list.h"


#define EXIT_COMMAND "exit"
#define MAXARGS 7 /* Comando + 5 argumentos opcionais + espaco para NULL */
#define BUFFER_SIZE 100
#define MAXPAR 2


int numchildren = 0, exit_flag=0;
list_t* proc_list;
pthread_mutex_t latch;
sem_t sem_proc, sem_monitor;

/*-----------------------------------------------------------------------*/
void mutex_lock() {
  if(pthread_mutex_lock(&latch) != 0)
  {
    fprintf(stderr, "Error in pthread_mutex_lock()\n");
    exit(EXIT_FAILURE);
  }
}

void mutex_unlock() {
  if(pthread_mutex_unlock(&latch) != 0)
  {
    fprintf(stderr, "Error in pthread_mutex_unlock()\n");
    exit(EXIT_FAILURE);
  }
}


void s_wait(sem_t *sem) {
	if(sem_wait(sem) != 0)
	{
		fprintf(stderr, "Error in sem_wait()\n");
		exit(EXIT_FAILURE);
	}
}

void s_post(sem_t *sem) {
	if(sem_post(sem) != 0)
	{
		fprintf(stderr, "Error in sem_post()\n");
		exit(EXIT_FAILURE);
	}
}
/*-----------------------------------------------------------------------*/


void *monitor_cicle() {
  int status = 0, childpid = 0;
  time_t end_time;

  while (1) {
    s_wait(&sem_monitor);
    mutex_lock();
    if (exit_flag && numchildren == 0) { /* verifica de a flag do exit ja se encontra activa e se nao ha filhos ativos*/
      mutex_unlock();
      pthread_exit(EXIT_SUCCESS);
    }

    else {
      mutex_unlock();
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
      s_post(&sem_proc);
      mutex_lock();
      update_terminated_process(proc_list, childpid, status, end_time); /* faz update na lista de filhos, adicionando-o */
      numchildren--;
      mutex_unlock();
    }
  }
}



int main (int argc, char** argv) {

  char *args[MAXARGS];
  char buffer[BUFFER_SIZE];
  pthread_t monitor;
  time_t start_time;
  int pid;

  sem_init(&sem_proc, 0, MAXPAR);
  sem_init(&sem_monitor, 0, 0);


  if(pthread_mutex_init(&latch, NULL) != 0) { /*cria o mutex latch*/
    fprintf(stderr, "Error creating mutex.\n");
    exit(EXIT_FAILURE);
  }

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
      mutex_lock();
      exit_flag=1;   				/* altera a flag do exit */
      mutex_unlock();
      s_post(&sem_monitor);
      if (pthread_join(monitor, NULL) != 0){	/* espera que a tarefa monitor acabe*/
	fprintf(stderr, "Error joining thread.\n");
	exit(EXIT_FAILURE);
	}		
      lst_print(proc_list);			/* faz print da lista de filhos*/
      lst_destroy(proc_list);			/* destroi a lista*/
      pthread_mutex_destroy(&latch);		/* destroi o mutex*/
	sem_destroy(&sem_proc);			/* destroi o semaforo sem_proc*/
	sem_destroy(&sem_monitor);		/* destroi o semaforo sem_monitor*/
      exit(EXIT_SUCCESS);
    }

    /* Caso tenha havido argumentos e nao seja "exit", lancamos processo filho */

    s_wait(&sem_proc);
    pid = fork(); /*cria um filho*/
    start_time = time(NULL); /*guarda o tempo de inicio da vida do filho*/

    if (pid < 0) {
	    perror("Failed to create new process.\n");
	    exit(EXIT_FAILURE);
    }

    if (pid > 0) { 	  /* Codigo do processo pai */
	s_post(&sem_monitor);
	mutex_lock();
      	insert_new_process(proc_list, pid, 0, start_time);
	numchildren++;
	mutex_unlock();
    }

    else { /* Codigo do processo filho */
	    if (execv(args[0], args) < 0) {
        perror("Could not run child program. Child will exit.\n");
        exit(EXIT_FAILURE);
	    }
    }
  }

}
