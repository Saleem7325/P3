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

//#include "node.h"
/*
typedef struct node{
	char *data;
	struct node *next;
}node;
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

//int is_empty(directory_queue *q);

void close_directory_stack(directory_stack *s);
//void request_exit(queue *q, int thread_count);
