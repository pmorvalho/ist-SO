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
  char buffer[BUFFER_SIZE];
  char pids[BUFFER_SIZE];
  int fileServ, fileClit;

  while(1) {
    if ((fileServ = open(argv[1], FIFO_MODE)) >= 0) {
      break;
    }
  }
  
  sprintf(pids, "p %d", getpid());
  write(fileServ, pids, BUFFER_SIZE);

  while(1) {
    char command[BUFFER_SIZE];
    
    if (fgets(buffer, BUFFER_SIZE-2, stdin) == NULL) {
      continue;
    }
    
    
    if(strcmp(buffer, EXIT_COMMAND) == 0){
      close(fileServ);
      exit(EXIT_SUCCESS);
    }
    
    
    sprintf(command, "c %s", buffer);
    write(fileServ, command, BUFFER_SIZE);
    
    if(strcmp(buffer, STATS_COMMAND) == 0){
      int n;
      fileClit = open(FIFO_STATUS, FIFO_MODE);
      strcpy(buffer, "");
      while(1){
	if ((n = read(fileClit, buffer, BUFFER_SIZE)) >= 0)
	  break;
      }
      printf(buffer);
      close(fileClit);
    }
    

  }
  
}
