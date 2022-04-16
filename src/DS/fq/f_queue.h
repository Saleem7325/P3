#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef struct node{
	char *data;
	struct node *next;
}node;

typedef struct queue{
	node *head;
	int empty;
	pthread_mutex_t lock;
	pthread_cond_t dequeue_ready;
}queue;

queue *init_queue(queue *q);

char *dequeue(queue *q);

void enqueue(queue *q, char *data);

int is_empty(queue *q);
