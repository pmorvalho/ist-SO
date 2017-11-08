/*
 * list.c - implementation of the integer list functions
 */


#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "list.h"



list_t* lst_new()
{
   list_t *list;
   list = (list_t*) malloc(sizeof(list_t));
   list->first = NULL;
   return list;
}


void lst_destroy(list_t *list)
{
	struct lst_iItem *item, *nextitem;

	item = list->first;
	while (item != NULL){
		nextitem = item->next;
		free(item);
		item = nextitem;
	}
	free(list);
}


void insert_new_process(list_t *list, int pid, int status, time_t startTime)
{
	lst_iItem_t *item;

	item = (lst_iItem_t *) malloc (sizeof(lst_iItem_t));
	item->pid = pid;
  item->status = status;
	item->startTime = startTime;
	item->endTime = 0;
	item->next = list->first;
	list->first = item;
}

void update_terminated_process(list_t *list, int pid, int status, time_t endTime)
{
  lst_iItem_t * aux = NULL;
  for(aux = list->first ; aux != NULL; aux = aux->next) {
    if (aux->pid == pid) {
      aux->status = status;
      aux->endTime = endTime;
      break;
    }
  }
}


void lst_print(list_t *list)
{
	lst_iItem_t *item;

	printf("Process list with start and end time:\n");
	item = list->first;
	/* while(1){ */ /* use it only to demonstrate gdb potencial */
	while (item != NULL){
    if (WIFEXITED(item->status))
      printf("pid: %d exited normally with run time %f seconds; status=%d\n", item->pid, difftime(item->endTime, item->startTime), WEXITSTATUS(item->status));
    else
      printf("pid: %d terminated without calling exit\n", item->pid);

		item = item->next;
	}
	printf("-- end of list.\n");
}
