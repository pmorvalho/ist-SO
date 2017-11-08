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
#define MAXARGS 7 /* Comando + 5 argumentos opcionais + espaco para NULL */
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



void trata_ctrl_c(int signum){   /* rotina que trata do SIGINT */
  exit_flag = 1;
  
  if(close(0)==-1) printf("Error closing file\n"); /*fecha o ficheiro de input, stdinput*/
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
      if(fflush(file)!=0) printf("Error with fflush\n");
      cond_signal(&cond_proc);
      mutex_unlock();
    }
  }
}

/*-----------------------------------------------------------------------*/

int main (int argc, char** argv) {

  char *args[MAXARGS];
  char buffer[BUFFER_SIZE];
  char line[LINE_MAX_SIZE];
  time_t start_time;
  int pid;
  
  /* caso receba o SIGINT chama a funcao para o tratar*/
  if(signal(SIGINT, trata_ctrl_c) == SIG_ERR ){perror("Error on signal handler");} 
  

  /************************ Cria e Abre Pipes **************************/

  /* cria fifo para receber os inputs das par-shell-terminals*/
  unlink(FIFO_INPUT);
  if (mkfifo(FIFO_INPUT, FIFO_MODE) < 0) { /* verifica o erro de criacao do fifo*/
    fprintf(stderr, "Error making fifo\n");
    exit(EXIT_FAILURE);
  }
  if ((fileServ = open(FIFO_INPUT, FIFO_MODE)) < 0) { /*verifica o erro da abertura*/
    fprintf(stderr, "Error opening par-shell-in\n");
    exit(EXIT_FAILURE);
  }
  
  /* cria um fifo para mandar informacao sobre os processos para as par-shell-terminais */
  unlink(FIFO_STATUS);
  if(mkfifo(FIFO_STATUS, FIFO_MODE) < 0) { /* veririfca o erro de criar o fifo */
    fprintf(stderr, "Error making fifo\n");
    exit(EXIT_FAILURE);
  }
  if ((fileClit = open(FIFO_STATUS, FIFO_MODE)) < 0) { /*verifica o erro de abrir o ficheiro*/
    fprintf(stderr, "Error opening par-shell-in\n");
    exit(EXIT_FAILURE);
  }
  
  /* cria listas */
  proc_list = lst_new(); /*cria lista de filhos*/
  pids_list = intlst_new();

  file = fopen(FILE_NAME, FILE_MODE);

  if(file==NULL){ /* verifica da abertura do ficheiro*/
    fprintf(stderr, "Error opening log.txt.\n");
    exit(EXIT_FAILURE);
  }


  while(fgets(line, LINE_MAX_SIZE, file) != NULL) {  /* ciclo que vai buscar a ultima iteracao e o ultimo tempo total de execucao
							  do ficheiro log.txt*/
    sscanf(line, "iteracao %d", &iteration);
    sscanf(line, "total execution time: %d s", &total_exec_time);
  }
 
    /************************ inicializa variaveis de condicao **************************/

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

  if (close(0)==-1) printf("Error closing file\n");  /*fecha o ficheiro de input, e verifica erro*/

  if (dup(fileServ)==-1) printf("Error with dup()\n"); /* poe o ficheiro fileServ como input*/

  if (close(fileServ)==-1) printf("Erro closing file\n");  /* fecha um ficheiro e verifica o erro*/

  printf("Insert your commands:\n");
  while (1) {
    int n, numargs = 0;
    char *s = " \r\n\t";
    char *token;
    int i;
    
    if(exit_flag == 0){
      if (((n = read(0, buffer, BUFFER_SIZE)) < 0)) /*read do pipe de comandos da par-shell-terminal*/
	      printf("\nError reading\n");
      
    /*-------------------------------------Trata de letras de controlo-------------------------------------------*/
      
      if (buffer[0] == 'p'){ /*verifica se a letra de controlo e "p" pois caso seja trata-se de um pid que foi enviado*/
	     int term_pid = 0;
	     for(i = 0; i<BUFFER_SIZE-1; i++) /* retira a letra de controlo do comando*/
	        buffer[i] = buffer[i+1]; 
	        term_pid = atoi(buffer);
	        insert_intlst(pids_list, term_pid); /*insere novo pid na lista*/
	        continue;
	     }
	
	   for(i = 0; i<BUFFER_SIZE-1; i++) /* retira a letra de controlo do comando*/
	   buffer[i] = buffer[i+1]; 

	   /*----------------------------------------------------------------------------------------------------------*/

	   token = strtok(buffer, s); /* parte a string numa serie de string usando como parametro o s*/
 
	   while( numargs < MAXARGS-1 && token != NULL ) {
	      args[numargs] = token;
	       numargs ++;
	      token = strtok(NULL, s);
	   }
      
	     for (i = numargs; i<MAXARGS; i++) {  /*limpa o args depois da posicao numargs*/
	       args[i] = NULL;
	     }

	/* Verifica se: se chegou ao EOF (end of file) do stdin
	  ou se chegou a ordem "exit". Em ambos os casos, termina
	  ordeiramente esperando pela terminacao dos filhos */
	
	if (numargs <= 0) /*caso o numero de argmentos sejam zero volta para o while*/
	  continue;
    }
    
      /************************ EXIT GLOBAL **************************/

    if ((strcmp(args[0], EXIT_COMMAND_GLOBAL) == 0) || (exit_flag == 1)) { /*verifica se foi introduzido o comando 'exit'*/
      int terminal_pid = 0;
      mutex_lock();
      exit_flag=1;   			      /* altera a flag do exit */
      cond_signal(&cond_monitor);
      mutex_unlock();
      if (pthread_join(monitor, NULL) != 0){  /* espera que a tarefa monitora acabe*/
        fprintf(stderr, "Error joining thread.\n");
        exit(EXIT_FAILURE);
      }

      if(fclose(file)!=0) printf("Error opening file\n"); /* faz close do file e verifica o respectivo erro*/


      lst_print(proc_list);			/* faz print da lista de filhos*/
      lst_destroy(proc_list);			/* destroi a lista*/
      pthread_mutex_destroy(&latch);		/* destroi o mutex*/
      pthread_cond_destroy(&cond_proc);  	/*destroi a variavel de condicao cond_proc */
      pthread_cond_destroy(&cond_monitor);  	/*destroi a variavel de condicao cond_monitor */
      
      if(close(fileClit)==-1) printf("Error closing file\n");  /* faz close do pipe e verifica o respectivo erro*/
      while(1){
	       if((terminal_pid = remove_intlst(pids_list))==-1) break; /* remove da lista de pids um pid de cada vez ate ja nao restsrem pids*/
	         kill(terminal_pid, SIGINT); /* envia um sigkill para casa pid da lista*/
      }
      exit(EXIT_SUCCESS);
    }
    

      /************************ STATS **************************/

    if(strcmp(args[0], STATS_COMMAND) == 0) { /* verifica se o comando recebido foi um stats*/
      strcpy(buffer, "");
      /* escreve no buffer a informacao e de seguida escreve-a no pipe*/
      sprintf(buffer, "Number of child processes: %d\nTotal execution time: %d\n", numchildren, total_exec_time);
      if( write(fileClit, buffer, BUFFER_SIZE)==-1) printf("Error with write\n"); 
      continue;
    }


      /************************ Cria filhos **************************/
    
    /* Caso tenha havido argumentos e nao seja "exit", lancamos processo filho */
    mutex_lock();
    while(numchildren==MAXPAR){ /* espera ate que haja pelo menos um processador livre*/
      cond_wait(&cond_proc, &latch);
    }
    mutex_unlock();
    pid = fork(); 	     /*cria um filho*/
    start_time = time(NULL); /*guarda o tempo de inicio da vida do filho*/

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
      int fd;
      sprintf(fileName, "par-shell-out-%d.txt", i);

      /* faz open do file para escrever as informacoes de cada processo*/
      if ((fd = open(fileName, O_WRONLY|O_CREAT|O_EXCL, mode))==-1) 
          printf("Error with open\n");

      /* fecha ficheiro de output */
      if (close(1)==-1) printf("Error closing file\n");

      /* faz com que o ficheiro fd seja o novo ficheiro de output */
      if (dup(fd)==-1) printf("Error with dup()\n");

      /* faz com que os "filhos" nao sejam afectados pelo SIGINT do pai*/
      if (signal(SIGINT,SIG_IGN)==SIG_ERR) printf("Error with signal\n");

      if (execv(args[0], args) < 0) { /*faz execv e verifica o respectivo erro*/
        perror("Could not run child program. Child will exit.\n");
        exit(EXIT_FAILURE);
	    }
    }
  }
}