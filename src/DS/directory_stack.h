#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#ifndef NODE_H
#define NODE_H 
typedef struct node{
	char *data;
	struct node *next;
}node;
#endif 

/*
* - head is the first node in stack
* - empty is a flag that indicates whether stack is empty
* - closed is a flag that indicates whether more values will be added to stack
* - threads is the number of threads that will be using the stack
* - active_threads is the number of threads that are not waiting to pop from stack 
* - lock is the lock needed in order to perform any operation stack
* - pop_ready is a signal sent to threads waiting to pop from empty stack
*/
typedef struct directory_stack{
	node *head;
	int empty;
	int closed;
	int threads;
	int active_threads;
	pthread_mutex_t lock;
	pthread_cond_t pop_ready;
}directory_stack;

directory_stack *init_directory_stack(directory_stack *s, int thread_count);

char *pop_d(directory_stack *s);

void push_d(directory_stack *s, char *data);

void close_directory_stack(directory_stack *s);
