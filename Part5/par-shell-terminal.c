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

#define EXIT_COMMAND "exit\n"
#define EXIT_COMMAND_GLOBAL "exit-global\n"
#define STATS_COMMAND "stats\n"
#define MAXARGS 7 /* Comando + 5 argumentos opcionais + espaco para NULL */
#define BUFFER_SIZE 100
#define FIFO_STATUS "/tmp/par-shell-stats"
#define FIFO_MODE 0666


int main(int argc, char** argv) {
  char buffer[BUFFER_SIZE]; /* inicializa o buffer*/
  char pids[BUFFER_SIZE];   /* inicializa o buffer pids, que vai enviar o pid do terminal quando esse faz o open do fifo */
  int fileServ, fileClit;   


  while(1) {
    if ((fileServ = open(argv[1], FIFO_MODE)) >= 0) { /* abre o pipe a partir do qual passamos os comandas a partir do par-shell-terminal para o par-shell */
      break;
    }
  }
  /* letra de controlo para a par-shell saber que o que esta a receber e um pid de um novo par-shell-termnial */
  sprintf(pids, "p %d", getpid()); /* junta o pid do terminal ao buffer pids*/
  if (write(fileServ, pids, BUFFER_SIZE)==-1) printf("Error with write\n");; /* escreve no pipe */

  while(1) {
    char command[BUFFER_SIZE];
    
    if (fgets(buffer, BUFFER_SIZE-2, stdin) == NULL) { /* recebe os comandos do terminal*/
      continue;
    }
    
     
    if(strcmp(buffer, EXIT_COMMAND) == 0){  /* verifica se o comando e exit*/
      if (close(fileServ)==-1) printf("Error closing\n");;
      exit(EXIT_SUCCESS);
    }
    
    /* letra de controlo para garantir que nenhum terminal manda um comando que posso ser interpretado por um novo pid*/
    sprintf(command, "c %s", buffer); /* junta o comando e a letra de controlo*/
    if( write(fileServ, command, BUFFER_SIZE)==-1) printf("Error with write\n");; /* escreve no buffer */
    
    if(strcmp(buffer, STATS_COMMAND) == 0){  /*verifica se o comando era o comando status*/
      int n;
      if ( (fileClit = open(FIFO_STATUS, FIFO_MODE))==-1) printf("Error opening\n");;   /* abre o fifo para receber a informacao da par-shell */
      strcpy(buffer, "");
      while(1){
	     if ((n = read(fileClit, buffer, BUFFER_SIZE)) >= 0)  /* le a informacao enviada pela par-shell*/
	     break;
      }
      printf(buffer);
      if (close(fileClit)==-1) printf("Error closing\n");;
    }
    

  }
  
}
