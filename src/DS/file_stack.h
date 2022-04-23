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


typedef struct file_stack{
	node *head;
	int empty;
	int closed;
	int threads;
	int active_threads;
	pthread_mutex_t lock;
	pthread_cond_t pop_ready;
}file_stack;

file_stack *init_file_stack(file_stack *s, int thread_count);

char *pop_f(file_stack *s);

void push_f(file_stack *s, char *data);

//int is_empty(file_queue *q);

void close_file_stack(file_stack *s);

//void request_exit(queue *q, int thread_count);
