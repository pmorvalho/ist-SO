/*
// Par-shell - exercicio 1, version 1
// Sistemas Operativos, DEI/IST/ULisboa 2015-16
*/

#include "commandlinereader.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define EXIT_COMMAND "exit"
#define MAXARGS 5
#define STATUS_INFO_LINE_SIZE 50
#define BUFFER_SIZE 100

int main (int argc, char** argv) {

  int numchildren = 0;
  char *args[MAXARGS + 1];

  char buffer[BUFFER_SIZE];

  while (1) {
    int numargs;
    
    printf("Insert your command: "); fflush(stdout);

    numargs = readLineArguments(args, MAXARGS, buffer, BUFFER_SIZE);

    /* Verifica se: se chegou ao EOF (end of file) do stdin
       ou se chegou a ordem "exit". Em ambos os casos, termina
       ordeiramente esperando pela terminacao dos filhos */
    if (numargs < 0 ||
	(numargs > 0 && (strcmp(args[0], EXIT_COMMAND) == 0))) {

	int status, childpid;
	int message_size = numchildren * STATUS_INFO_LINE_SIZE;

	char *childstatusmessage = NULL;
	char aux[STATUS_INFO_LINE_SIZE];

	/* Aloca array de carateres onde sera' adicionada informacao sobre cada 
	   filho terminado */
	if (numchildren > 0) {
	  childstatusmessage = (char*)malloc(message_size);
	  childstatusmessage[0] = '\0';
	}
	
	/* Espera pela terminacao de cada filho */
	while (numchildren > 0) {
	  childpid = wait(&status);

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
	  
	  /* Um filho terminou, logo adicionamos uma linha 'a string childstatusmessage
	     sobre este filho */
	  if (WIFEXITED(status))
	    snprintf(aux, sizeof(aux), "pid: %d exited normally; status=%d\n", 
		     childpid, WEXITSTATUS(status));
	  else
	    snprintf(aux, sizeof(aux), "pid: %d terminated without calling exit\n", 
		     childpid);
	  strncat(childstatusmessage, aux, message_size);
	  
	  numchildren --;
 	} 

	/* Neste ponto, todos os filhos terminaram, logo finalmente imprimimos e 
	   terminamos o processo da shell */
	if (childstatusmessage != NULL) 
	  printf("%s", childstatusmessage);
	exit(EXIT_SUCCESS);
    }

    /* Caso tenha havido argumentos e nao seja "exit", lancamos processo filho */
    else if (numargs > 0) {
      
	int pid = fork();

	if (pid < 0) {
	  perror("Failed to create new process.");
	  exit(EXIT_FAILURE);
	}

	if (pid > 0) { 	  /* Codigo do processo pai */
	  numchildren ++;
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

