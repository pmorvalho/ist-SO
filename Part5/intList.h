/*
 * intList.h - definitions and declarations of the integer list
 */

#ifndef INTLIST_H
#define INTLIST_H

#include <stdlib.h>
#include <stdio.h>



/* lst_intItem - each element of the list points to the next element */
typedef struct lst_intItem{  
   int pid;
   struct lst_intItem *next;
} lst_intItem_t;

/* list_t */
typedef struct {
   lst_intItem_t *first;
} intList_t;


/* lst_new - allocates memory for list_t and initializes it */
intList_t* intlst_new();

/* insert um novo int (pid) na lista */
void insert_intlst(intList_t *list, int pid);

/* remove um elemento int (pid) da lista */
int remove_intlst(intList_t *list);

#endif
