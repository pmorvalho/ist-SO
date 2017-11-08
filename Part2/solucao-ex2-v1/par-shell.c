/*
 * Par-shell - exercicio 2, version 1
 * Sistemas Operativos, DEI/IST/ULisboa 2015-16
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#include "commandlinereader.h"
#include "list.h"


#define EXIT_COMMAND "exit"

#define MAXARGS        7
#define BUFFER_SIZE  100


int num_children = 0;
int flag_exit = 0; /* do not exit */

list_t *proc_data;

pthread_mutex_t data_ctrl;



/* 
+-----------------------------------------------------------------------*/
void mutex_lock(void) {
  if(pthread_mutex_lock(&data_ctrl) != 0)
  {
    fprintf(stderr, "Error in pthread_mutex_lock()\n");
    exit(EXIT_FAILURE);
  }
}


/* 
+-----------------------------------------------------------------------*/
void mutex_unlock(void) {
  if(pthread_mutex_unlock(&data_ctrl) != 0)
  {
    fprintf(stderr, "Error in pthread_mutex_unlock()\n");
    exit(EXIT_FAILURE);
  }
}


/* 
+-----------------------------------------------------------------------*/
void *tarefa_monitora(void *arg_ptr) {
  int status, childpid;
  time_t end_time;


  printf(" *** Tarefa monitora activa.\n");

  while(1) {
    mutex_lock();
    if(num_children == 0) {
      if(flag_exit == 1) {
	mutex_unlock();
	printf(" *** Tarefa monitora terminou.\n");
	pthread_exit(NULL);
      }
      else {
	mutex_unlock();
	/* printf(" *** No children running.\n"); */
	sleep(1); /* 1 second */
	continue;
      }
    }
    mutex_unlock();

    /* wait for a child process */

    childpid = wait(&status);
    if (childpid == -1) {
      perror("Error waiting for child");
      exit(EXIT_FAILURE);
    }

    end_time = time(NULL);

    mutex_lock();
    num_children --;
    update_terminated_process(proc_data, childpid, end_time, status);
    mutex_unlock();
  }
}


/* 
+-----------------------------------------------------------------------*/
int main (int argc, char** argv) {
  pthread_t tid;
  char buffer[BUFFER_SIZE];
  int numargs;
  char *args[MAXARGS];
  time_t start_time;
  int pid;


  /* criar estrutura de dados de monitorização */
  proc_data = lst_new();

  /* criar mutex */
  if(pthread_mutex_init(&data_ctrl, NULL) != 0) {
    fprintf(stderr, "Error creating mutex.\n");
    exit(EXIT_FAILURE);
  }

  /* criar thread */
  if (pthread_create(&tid, NULL, tarefa_monitora, NULL) != 0) {
    fprintf(stderr, "Error creating thread.\n");
    exit(EXIT_FAILURE);
  }


  while(1) {
    printf("Insert your command: ");
    numargs = readLineArguments(args, MAXARGS, buffer, BUFFER_SIZE);

    if (numargs < 0) {
      printf("Error in readLineArguments()\n");
      continue;
    }
    if (numargs == 0) /* empty line; read another one */
      continue;

    if (strcmp(args[0], EXIT_COMMAND) == 0) {
      printf("Ending...\n");
      break;
    }

    /* process a command */

    start_time = time(NULL);

    pid = fork();
    if (pid == -1) {
      perror("Failed to create new process.");
      exit(EXIT_FAILURE);
    }

    if (pid > 0) {  /* parent */
      mutex_lock();
      num_children ++;
      insert_new_process(proc_data, pid, start_time);
      mutex_unlock();
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
  mutex_lock();
  flag_exit = 1;
  mutex_unlock();

  /* wait for thread to end */
  if(pthread_join(tid, NULL) != 0) {
    fprintf(stderr, "Error joining thread.\n");
    exit(EXIT_FAILURE);
  }

  lst_print(proc_data);

  /* clean up and exit */
  pthread_mutex_destroy(&data_ctrl);
  lst_destroy(proc_data);
  return 0; /* ok */
}

