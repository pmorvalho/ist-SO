/*
 * intList.c - implementation of the integer list functions
 */


#include <stdlib.h>
#include <stdio.h>
#include "intList.h"



intList_t* intlst_new()
{
   intList_t *list;
   list = (intList_t*) malloc(sizeof(intList_t));
   list->first = NULL;
   return list;
}

void insert_intlst(intList_t *list, int pid)
{
	lst_intItem_t *item;

	item = (lst_intItem_t *) malloc (sizeof(lst_intItem_t));
	item->pid = pid;
	item->next = list->first;
	list->first = item;
}

int remove_intlst(intList_t *list)
{
  int pid=0;
  lst_intItem_t *aux = NULL;
  lst_intItem_t *ap= NULL;
  if(list->first->next != NULL){
  aux = list->first->next;
  ap = list->first;
  list->first = aux;
  pid = ap->pid;
  free(ap);
  return pid;
  }
  else if (list->first != NULL){
  	pid = list->first->pid;
  	free(list->first);
  	return pid;
  }
  else return -1;
}

