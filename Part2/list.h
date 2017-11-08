/*
 * list.h - definitions and declarations of the integer list
 */

#ifndef LIST_H
#define LIST_H

#include <stdlib.h>
#include <stdio.h>
#include <time.h>



/* lst_iItem - each element of the list points to the next element */
typedef struct lst_iItem {
   int pid;
   int status;
   time_t startTime;
   time_t endTime;
   struct lst_iItem *next;
} lst_iItem_t;

/* list_t */
typedef struct {
   lst_iItem_t * first;
} list_t;


/* lst_new - allocates memory for list_t and initializes it */
list_t* lst_new();

/* lst_destroy - free memory of list_t and all its items */
void lst_destroy(list_t *);

/* insert_new_process - insert a new item with process id and its start time in list 'list' */
void insert_new_process(list_t *list, int pid, int status, time_t startTime);

/* update_teminated_process - updates endTime of element with pid 'pid' */
void update_terminated_process(list_t *list, int pid, int status ,time_t endTime);

/* lst_print - print the content of list 'list' to standard output */
void lst_print(list_t *list);

#endif
