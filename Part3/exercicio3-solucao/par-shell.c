/*
 * Par-shell - exercicio 3, version 2
 * Sistemas Operativos, DEI/IST/ULisboa 2015-16
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#include "commandlinereader.h"
#include "list.h"

#define EXIT_COMMAND "exit"
#define MAXARGS        7
#define BUFFER_SIZE  100
#define MAXPAR 2

/*****************************************************
 * Global variables. *********************************
 *****************************************************/
int g_num_children = 0;
int g_flag_exit = 0; /* do not exit */
list_t *g_proc_data;
pthread_mutex_t g_data_ctrl;
int g_max_concurrency;
sem_t g_concurrency_sem;
sem_t g_monitor_cycle_sem;

/*****************************************************
 * Monitor task function. ****************************
 *****************************************************/
void *tarefa_monitora(void *arg_ptr) {
  int status, childpid;
  time_t end_time;


  printf(" *** Tarefa monitora activa.\n");

  while(1) {
    
    if(sem_wait(&g_monitor_cycle_sem)) {
      perror("Error performing sem_wait on semaphore g_monitor_cycle_sem");
      exit(EXIT_FAILURE);
    }
    pthread_mutex_lock(&g_data_ctrl);
    if(g_flag_exit == 1 && g_num_children == 0) {
    	pthread_mutex_unlock(&g_data_ctrl);
    	printf(" *** Tarefa monitora terminou.\n");
      break;
    }
    pthread_mutex_unlock(&g_data_ctrl);
    
    /* wait for a child process */
    childpid = wait(&status);
    if (childpid == -1) {
      perror("Error waiting for child. Thread will end.");
      exit(EXIT_FAILURE);
    }

    end_time = time(NULL);

    pthread_mutex_lock(&g_data_ctrl);
    g_num_children --;
    update_terminated_process(g_proc_data, childpid, end_time, status);
    pthread_mutex_unlock(&g_data_ctrl);

    if(g_max_concurrency > 0 && sem_post(&g_concurrency_sem) == -1) {
      perror("Error incrementing semaphore g_concurrency_sem. Thread will end.");
      exit(EXIT_FAILURE);
    }
  }

  pthread_exit(NULL);
}


/*****************************************************
 * Main thread. **************************************
 *****************************************************/
int main (int argc, char** argv) {
  pthread_t tid;
  char buffer[BUFFER_SIZE];
  int numargs;
  char *args[MAXARGS];
  time_t start_time;
  int pid;

  if(argc != 1) {
    printf("Invalid argument count.\n");
    printf("Usage:\n");
    printf("\t%s\n\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  g_max_concurrency = MAXPAR;

  /* Inicializar semforo de limite de concurrencia dos filhos. */
  if (g_max_concurrency > 0 && sem_init(&g_concurrency_sem, 0, g_max_concurrency) == -1) {
    perror("Could not initialize semaphore");
    exit(EXIT_FAILURE);
  }

  /* Inicializar semaforo da thread monitora a 0 */
  if (sem_init(&g_monitor_cycle_sem, 0, 0) == -1) {
    perror("Could not initialize semaphore");
    exit(EXIT_FAILURE);
  }

  /* criar estrutura de dados de monitorização */
  g_proc_data = lst_new();


  /* criar mutex */

  if(pthread_mutex_init(&g_data_ctrl, NULL) != 0) {
    perror("Could not create mutex");
    exit(EXIT_FAILURE);
  }


  /* criar thread */

  if (pthread_create(&tid, NULL, tarefa_monitora, NULL) != 0) {
    printf("Error creating thread.\n");
    exit(EXIT_FAILURE);
  }

  printf("Concurrencia limite de processos filho: ");//%d\n", g_max_concurrency);
  (g_max_concurrency == 0) ? printf("%d (sem limite)\n", g_max_concurrency) : printf("%d\n", g_max_concurrency);
  while(1) {
    pthread_mutex_lock(&g_data_ctrl);
    if(!g_num_children) {
      printf("Inserir comando: ");
    }
    pthread_mutex_unlock(&g_data_ctrl);

    numargs = readLineArguments(args, MAXARGS, buffer, BUFFER_SIZE);

    if (numargs < 0) {
      printf("Error in readLineArguments()\n");
      continue;
    }
    if (numargs == 0) /* empty line; read another one */
      continue;

    if (strcmp(args[0], EXIT_COMMAND) == 0) {
      printf("Ending...\n");
      if(sem_post(&g_monitor_cycle_sem) == -1) {
        perror("Error incrementing semaphore g_monitor_cycle_sem. Thread will end.");
        exit(EXIT_FAILURE);
      }
      break;
    }

    /* process a command */

    /* If there are g_max_concurrency processes running at the moment, 
     * wait for the motitor thread X to check one of their exits.
     */
    if(g_max_concurrency > 0 && sem_wait(&g_concurrency_sem)) {
      perror("Error performing sem_wait on semaphore g_concurrency_sem");
      exit(EXIT_FAILURE);
    }

    start_time = time(NULL);

    pid = fork();
    if (pid == -1) {
      perror("Failed to create new process.");
      exit(EXIT_FAILURE);
    }

    if (pid > 0) {  /* parent */
      pthread_mutex_lock(&g_data_ctrl);
      g_num_children ++;
      insert_new_process(g_proc_data, pid, start_time);

      pthread_mutex_unlock(&g_data_ctrl);

      if(sem_post(&g_monitor_cycle_sem) == -1) {
        perror("Error incrementing semaphore g_monitor_cycle_sem. Thread will end.");
        exit(EXIT_FAILURE);
      }
    }
    else if (pid == 0) {  /* child */
      if (execv(args[0], args) == -1) {
      	perror("Could not run child program. Child will exit.");
      	exit(EXIT_FAILURE);
      }
    }
  }

  /* received command exit */

  /* request the monitoring thread to end */
  pthread_mutex_lock(&g_data_ctrl);
  g_flag_exit = 1;
  pthread_mutex_unlock(&g_data_ctrl);

  /* wait for thread to end */
  if(pthread_join(tid, NULL) != 0) {
    printf("Error joining thread.\n");
    exit(EXIT_FAILURE);
  }

  lst_print(g_proc_data);

  /* clean up and exit */
  pthread_mutex_destroy(&g_data_ctrl);
  
  if(g_max_concurrency > 0 && sem_destroy(&g_concurrency_sem)) {
    perror("Error destroying semaphore g_concurrency_sem");
    exit(EXIT_FAILURE);
  }
  
  if(sem_destroy(&g_monitor_cycle_sem)) {
    perror("Error destroying semaphore g_monitor_cycle_sem");
    exit(EXIT_FAILURE);
  }

  lst_destroy(g_proc_data);
  return 0; /* ok */
}

