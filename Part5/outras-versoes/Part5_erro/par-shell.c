/*
SHELL PARALELA - EXERCICIO 5
Grupo 40:
81151 - Pedro Orvalho
81365 - Ana Leitao
81647 - Manuel Galamba
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include "commandlinereader.h"
#include "list.h"
#include "intList.h"
#include <signal.h>


#define EXIT_COMMAND_GLOBAL "exit-global"
#define STATS_COMMAND "stats"
#define MAXARGS 8 /* Comando + 5 argumentos opcionais + espaco para NULL */
#define BUFFER_SIZE 100
#define MAXPAR 2
#define LINE_MAX_SIZE 50
#define FIFO_STATUS "/tmp/par-shell-stats"
#define FIFO_INPUT "/tmp/par-shell-in"
#define FIFO_MODE 0666
#define FILE_NAME "log.txt"
#define FILE_MODE "a+"



int numchildren = 0, exit_flag=0;
list_t* proc_list; 
intList_t* pids_list;
pthread_mutex_t latch;
pthread_cond_t cond_proc, cond_monitor ;
FILE *file;
int iteration = -1, total_exec_time = 0;
mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
int fileServ, fileClit;
pthread_t monitor;



/*-----------------------------------------------------------------------*/
void mutex_lock() {    /* testa o lock do mutex */
  if(pthread_mutex_lock(&latch) != 0) {
    fprintf(stderr, "Error in pthread_mutex_lock()\n");
    exit(EXIT_FAILURE);
  }
}

void mutex_unlock() {  /* testa o unlock do mutex*/
  if(pthread_mutex_unlock(&latch) != 0) {
    fprintf(stderr, "Error in pthread_mutex_unlock()\n");
    exit(EXIT_FAILURE);
  }
}


void cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) { /* testa o wait da variavel de condicao*/
  if(pthread_cond_wait(cond, mutex) != 0) {
    perror("Error in pthread_cond_wait()\n");
    exit(EXIT_FAILURE);
  }
}

void cond_signal(pthread_cond_t *cond) { /* testa o wait da variavel de condicao*/
  if(pthread_cond_signal(cond) != 0) {
    perror("Error in pthread_cond_signal()\n");
    exit(EXIT_FAILURE);
  }
}
/*-----------------------------------------------------------------------*/



void trata_ctrl_c(int signum){
  exit_flag = 1;
  close(0);
}

/*-----------------------------------------------------------------------*/


void *monitor_cicle() {
  int status = 0, childpid = 0;
  time_t end_time;
  float exec_time;

  while (1) {
    mutex_lock();

    while((numchildren == 0)&&(exit_flag==0)){  /* espera ate que haja processos por tratar*/
      cond_wait(&cond_monitor, &latch);
    }

    if (exit_flag && numchildren == 0) { /* verifica se a flag do exit ja se encontra activa e se nao ha filhos ativos*/
      mutex_unlock();
      pthread_exit(EXIT_SUCCESS);
    }

    else {
      mutex_unlock();
      childpid = wait(&status);
      end_time = time(NULL);   /*guarda o tempo de fim de vida do filho*/

      if (childpid < 0) {
        if (errno == EINTR) {
          mutex_lock();
          mutex_unlock();
          /* Este codigo de erro significa que chegou signal que interrompeu a espera
          pela terminacao de filho; logo voltamos a esperar */
          continue;
        }
        else {
          perror("Error waiting for child.");
          exit(EXIT_FAILURE);
        }
      }
      mutex_lock();
      numchildren--;
      exec_time = update_terminated_process(proc_list, childpid, status, end_time); /* faz update na lista de filhos, adicionando-o */
      total_exec_time += exec_time;
      fprintf(file, "iteracao %d\npid: %d execution time: %.0f s\ntotal execution time: %d s\n", ++iteration, childpid, exec_time, total_exec_time); /*faz a escrita no ficheiro sobre a
																		     nova iteracao e o tempo toal de execucao*/
      fflush(file);
      cond_signal(&cond_proc);
      mutex_unlock();
    }
  }
}

/*-----------------------------------------------------------------------*/

int main () {

  char *args[MAXARGS];
  char buffer[BUFFER_SIZE];
  char line[LINE_MAX_SIZE];
  time_t start_time;
  int pid;
  
  
  if(signal(SIGINT, trata_ctrl_c) == SIG_ERR ){perror("Erro no signal handler");}
  

  unlink(FIFO_INPUT);
  if (mkfifo(FIFO_INPUT, FIFO_MODE) < 0) {
    fprintf(stderr, "Error making fifo\n");
    exit(EXIT_FAILURE);
  }
  if ((fileServ = open(FIFO_INPUT, FIFO_MODE)) < 0) {
    fprintf(stderr, "Error opening par-shell-in\n");
    exit(EXIT_FAILURE);
  }
  
  
  unlink(FIFO_STATUS);
  if(mkfifo(FIFO_STATUS, FIFO_MODE) < 0) {
    fprintf(stderr, "Error making fifo\n");
    exit(EXIT_FAILURE);
  }
  if ((fileClit = open(FIFO_STATUS, FIFO_MODE)) < 0) {
    fprintf(stderr, "Error opening par-shell-in\n");
    exit(EXIT_FAILURE);
  }
  
  
  proc_list = lst_new(); /*cria lista de filhos*/
  pids_list = intlst_new();

  file = fopen(FILE_NAME, FILE_MODE);

  if(file==NULL){
    fprintf(stderr, "Error opening log.txt.\n");
    exit(EXIT_FAILURE);
  }


  while(fgets(line, LINE_MAX_SIZE, file) != NULL) {  /* ciclo que vai buscar a ultima iteracao e o ultimo tempo total de execucao
							  do ficheiro log.txt*/
    sscanf(line, "iteracao %d", &iteration);
    sscanf(line, "total execution time: %d s", &total_exec_time);
  }


  if(pthread_cond_init(&cond_proc, NULL)!=0){ /*verifica se a variavel de condicao cond_proc foi criada*/
    fprintf(stderr, "Error creating condition variable, cond_proc.\n");
    exit(EXIT_FAILURE);
  }

  if(pthread_cond_init(&cond_monitor, NULL)!=0){ /*verifica se a variavel de condicao cond_monitor foi criada*/
    fprintf(stderr, "Error creating condition variable, cond_monitor.\n");
    exit(EXIT_FAILURE);
  }

  if(pthread_mutex_init(&latch, NULL) != 0) { /*verifica se o mutex foi criado*/
    fprintf(stderr, "Error creating mutex.\n");
    exit(EXIT_FAILURE);
  }

  if (pthread_create(&monitor, 0, monitor_cicle, NULL) != 0) { /*verifica se a tarefa monitor foi criada*/
    printf("Error with pthread_create!\n");
    exit(EXIT_FAILURE);
  }

  close(0);
  dup(fileServ);
  close(fileServ);

  printf("Insert your commands:\n");
  while (1) {
    int numargs = 0;
    printf("OLa");
    /*if(exit_flag == 0){}*/
      
      numargs = readLineArguments(args, MAXARGS, buffer, BUFFER_SIZE);
      if (numargs <= 1)
	  continue;
      
      if (strcmp(args[0],"p")==0){
	int term_pid = 0;
	term_pid = atoi(args[1]);
	printf("%s\n",args[0]);
	insert_intlst(pids_list, term_pid);
	continue;
	}

	/* Verifica se: se chegou ao EOF (end of file) do stdin
	  ou se chegou a ordem "exit". Em ambos os casos, termina
	  ordeiramente esperando pela terminacao dos filhos */
   
    	printf("%s\n",args[1]);
    if ((strcmp(args[1], EXIT_COMMAND_GLOBAL) == 0) || (exit_flag == 1)) { /*verifica se foi introduzido o comando 'exit'*/
      int terminal_pid = 0;
      mutex_lock();
      exit_flag=1;   			      /* altera a flag do exit */
      cond_signal(&cond_monitor);
      mutex_unlock();
      if (pthread_join(monitor, NULL) != 0){  /* espera que a tarefa monitor acabe*/
        fprintf(stderr, "Error joining thread.\n");
        exit(EXIT_FAILURE);
      }

      fclose(file);

      lst_print(proc_list);			/* faz print da lista de filhos*/
      lst_destroy(proc_list);			/* destroi a lista*/
      pthread_mutex_destroy(&latch);		/* destroi o mutex*/
      pthread_cond_destroy(&cond_proc);  	/*destroi a variavel de condicao cond_proc */
      pthread_cond_destroy(&cond_monitor);  	/*destroi a variavel de condicao cond_monitor */
      close(fileClit);
     while(1){
	      if((terminal_pid = remove_intlst(pids_list))==-1) break;
	       kill(terminal_pid, SIGINT);
        }
      exit(EXIT_SUCCESS);
    }
    
    if(strcmp(args[1], STATS_COMMAND) == 0) {
      strcpy(buffer, "");
      sprintf(buffer, "Number of child processes: %d\nTotal execution time: %d\n", numchildren, total_exec_time);
      write(fileClit, buffer, BUFFER_SIZE);
      continue;
    }
    	printf("print2%s\n",args[1]);
    /* Caso tenha havido argumentos e nao seja "exit", lancamos processo filho */
    mutex_lock();
    while(numchildren==MAXPAR){ /* espera ate que haja pelo menos um processador livre*/
      cond_wait(&cond_proc, &latch);
    }
    mutex_unlock();
    pid = fork(); 	     /*cria um filho*/
    start_time = time(NULL); /*guarda o tempo de inicio da vida do filho*/
    	printf("print3%s\n",args[1]);
    if (pid < 0) {
      perror("Failed to create new process.\n");
      exit(EXIT_FAILURE);
    }

    if (pid > 0) { /* Codigo do processo pai */
      mutex_lock();
      insert_new_process(proc_list, pid, 0, start_time);
      numchildren++;
      cond_signal(&cond_monitor);
      mutex_unlock();
    }

    else { /* Codigo do processo filho */
      char fileName[100];
      int i = getpid();
      int j;
      int fd;
      sprintf(fileName, "par-shell-out-%d.txt", i);
      fd = open(fileName, O_WRONLY|O_CREAT|O_EXCL, mode);
      close(1);
      dup(fd);
      signal(SIGINT,SIG_IGN);
      for(j = 0; j<numargs-1; j++) /* retira a letra de controlo do comando*/
	  args[j] =  args[j+1];
      args[j]=NULL;
      if (execv(args[0], args) < 0) {
        perror("Could not run child program. Child will exit.\n");
        exit(EXIT_FAILURE);
	    }
    }
  }
}