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
  update_terminated_process(proc_list, childpid, status, time(NULL));


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
    snprintf(aux, sizeof(aux), "pid: %d exited normally; status=%d\n", childpid, WEXITSTATUS(status));
  else
    snprintf(aux, sizeof(aux), "pid: %d terminated without calling exit\n", childpid);
  strncat(childstatusmessage, aux, message_size);

  numchildren --;
}

/* Neste ponto, todos os filhos terminaram, logo finalmente imprimimos e
terminamos o processo da shell */
if (childstatusmessage != NULL) { printf("%s", childstatusmessage); }
