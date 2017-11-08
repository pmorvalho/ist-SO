/*
 * intList.c - implementation of the integer list functions
 */


#include <stdlib.h>
#include <stdio.h>
#include "intList.h"



intList_t* intlst_new()   /* cria uma lista de ligada de inteiros*/
{
   intList_t *list;
   list = (intList_t*) malloc(sizeof(intList_t));
   list->first = NULL;
   return list;
}

void insert_intlst(intList_t *list, int pid) /* insere um novo elemento na lista ligada de inteiros */
{
	lst_intItem_t *item;

	item = (lst_intItem_t *) malloc (sizeof(lst_intItem_t));
	item->pid = pid;
	item->next = list->first;
	list->first = item;
}

int remove_intlst(intList_t *list) /* remove um elemento da lista ligada de inteiros */
{
  int pid=0;
  lst_intItem_t *aux = NULL;
  lst_intItem_t *ap= NULL;
  if(list->first != NULL){ /* verifica se a lista tem um elemento pelo menos */
    aux = list->first->next;
    ap = list->first;
    list->first = aux;
    pid = ap->pid;
    ap->next=NULL;
    free(ap);
    return pid;
  }
  else 
    free(list); /* quando a lista esta vazia */
    return -1;
}

