/*
 * Par-shell - exercicio 4, version 1
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

#define EXIT_COMMAND   "exit"
#define MAXARGS        7
#define BUFFER_SIZE    100
#define MAXPAR         2
#define WRITING_PERIOD 10
#define LOG_FILE       "log.txt"

/*****************************************************
 * Global variables. *********************************
 *****************************************************/

int i;
int total = 0;
int max_concurrency;
int num_children = 0;
int flag_exit = 0;
list_t *proc_data;
pthread_mutex_t data_ctrl;
pthread_cond_t max_concurrency_ctrl;
pthread_cond_t no_command_ctrl;

/*****************************************************
 * Helper functions. *********************************
 *****************************************************/

void m_lock(pthread_mutex_t* mutex) {
  if (pthread_mutex_lock(mutex)) {
    perror("Error locking mutex");
    exit(EXIT_FAILURE);
  }
}

void m_unlock(pthread_mutex_t* mutex) {
  if (pthread_mutex_unlock(mutex)) {
    perror("Error unlocking mutex");
    exit(EXIT_FAILURE);
  }
}

void c_wait(pthread_cond_t* condition, pthread_mutex_t* mutex) {
  if (pthread_cond_wait(condition, mutex)) {
    perror("Error waiting on condition");
    exit(EXIT_FAILURE);
 }
}

void c_signal(pthread_cond_t* condition) {
  if (pthread_cond_signal(condition)) {
    perror("Error signaling on condition");
    exit(EXIT_FAILURE);
  }
}

/*****************************************************
 * Monitor task function. ****************************
 *****************************************************/
void *monitor(void *arg_ptr) {
  FILE *log_file;
  int status, pid;
  time_t end_time;

  log_file = fopen(LOG_FILE, "a");
  if (log_file == NULL) {
    perror("Error opening file");
    exit(EXIT_FAILURE);
  }

  while (1) {
    /*wait for effective command condition*/
    m_lock(&data_ctrl);
    while (num_children == 0 && flag_exit == 0) {
      c_wait(&no_command_ctrl, &data_ctrl);
    }
    if (flag_exit == 1 && num_children == 0) {
      m_unlock(&data_ctrl);
      break;
    }
    m_unlock(&data_ctrl);

    /*wait for child*/
    pid = wait(&status);
    if (pid == -1) {
      perror("Error waiting for child");
      exit(EXIT_FAILURE);
    }

    /*register child performance and signal concurrency condition*/
    end_time = time(NULL);
    m_lock(&data_ctrl);
    --num_children;
    update_terminated_process(proc_data, pid, end_time, status);
    int duration = end_time - process_start(proc_data, pid);
    if (max_concurrency > 0) {
      c_signal(&max_concurrency_ctrl);
    }
    m_unlock(&data_ctrl);

    /*print execution time to disk*/
    ++i;
    total += duration;
    fprintf(log_file, "iteracao %d\npid: %d execution time: %d s\ntotal execution time: %d s\n", i, pid, duration, total);
    if (fflush(log_file)) {
      perror("Error flushing file");
      exit(EXIT_FAILURE);
    }
  }

  if (fclose(log_file)) {
    perror("Error closing file");
    exit(EXIT_FAILURE);
  }

  pthread_exit(NULL);
}

/*****************************************************
 * Main thread. **************************************
 *****************************************************/
int main(int argc, char **argv) {
  pthread_t monitor_tid;
  char buffer[BUFFER_SIZE];
  int numargs;
  char *args[MAXARGS];
  time_t start_time;
  int pid, duration;
  FILE *log_file;

  if (argc != 1 && argc != 2) {
    printf("Invalid argument count.\n");
    printf("Usage:\n");
    printf("\t%s [MAXPAR]\n\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  max_concurrency = MAXPAR;
  if (argc == 2) {
    max_concurrency = atoi(argv[1]);
    if (max_concurrency < 0) {
      printf("Invalid maximum concurrency - must be positive integer.\n");
      exit(EXIT_FAILURE);
    }
  }

  /*initialize condition variables*/
  if (max_concurrency > 0 && pthread_cond_init(&max_concurrency_ctrl, NULL)) {
    perror("Error initializing condition");
    exit(EXIT_FAILURE);
  }

  if (pthread_cond_init(&no_command_ctrl, NULL)) {
    perror("Error initializing condition");
    exit(EXIT_FAILURE);
  }

  proc_data = lst_new();

  /*initialize proc_data*/
  log_file = fopen(LOG_FILE, "r");
  if (log_file == NULL) {
    i = -1; /*will be incremented later*/
  }

  if (log_file) {
    while (fgets(buffer, BUFFER_SIZE, log_file)) {
      if (sscanf(buffer, "iteracao %d", &i)) {
        continue;
      }
      if (sscanf(buffer, "total execution time: %d", &duration)) {
        continue;
      }
      if (sscanf(buffer, "pid: %d execution time: %d s", &pid, &duration)) {
        total += duration;
      }
    }

    if (fclose(log_file)) {
      perror("Error closing file");
      exit(EXIT_FAILURE);
    }
  }

  /*initialize mutex*/
  if (pthread_mutex_init(&data_ctrl, NULL)) {
    perror("Error initializing mutex");
    exit(EXIT_FAILURE);
  }

  /*create additional threads*/
  if (pthread_create(&monitor_tid, NULL, monitor, NULL) != 0) {
    perror("Error creating thread");
    exit(EXIT_FAILURE);
  }

  printf("Child processes concurrency limit: %d", max_concurrency);
  (max_concurrency == 0) ? printf(" (sem limite)\n\n") : printf("\n\n");
  while (1) {
    numargs = readLineArguments(args, MAXARGS, buffer, BUFFER_SIZE);

    if (numargs < 0) {
      printf("Error reading arguments\n");
      continue;
    }
    if (numargs == 0) {
      continue;
    }

    if (strcmp(args[0], EXIT_COMMAND) == 0) {
      break;
    }

    if (max_concurrency > 0) {
      m_lock(&data_ctrl);
      while (num_children == max_concurrency) {
        c_wait(&max_concurrency_ctrl, &data_ctrl);
      }
      m_unlock(&data_ctrl);
    }

    start_time = time(NULL);

    /*create child process*/
    pid = fork();
    if (pid == -1) {
      c_signal(&max_concurrency_ctrl);
      continue;
    }
    if (pid > 0) {
      m_lock(&data_ctrl);
      ++num_children;
      insert_new_process(proc_data, pid, start_time);
      c_signal(&no_command_ctrl);
      m_unlock(&data_ctrl);
    }
    if (pid == 0) {
      if (execv(args[0], args) == -1) {
        perror("Error executing command");
        exit(EXIT_FAILURE);
      }
    }
  }

  m_lock(&data_ctrl);
  flag_exit = 1;
  c_signal(&no_command_ctrl);
  m_unlock(&data_ctrl);

  /*synchronize with additional threads*/
  if (pthread_join(monitor_tid, NULL)) {
    perror("Error joining thread");
    exit(EXIT_FAILURE);
  }

  lst_print(proc_data);

  /*clean up*/
  pthread_mutex_destroy(&data_ctrl);
  if (max_concurrency > 0 && pthread_cond_destroy(&max_concurrency_ctrl)) {
    perror("Error destroying condition");
    exit(EXIT_FAILURE);
  }
  if(pthread_cond_destroy(&no_command_ctrl)) {
    perror("Error destroying condition");
    exit(EXIT_FAILURE);
  }
  lst_destroy(proc_data);

  return 0;
}
