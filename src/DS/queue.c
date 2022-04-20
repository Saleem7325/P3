#include "queue.h"

queue *init_queue(queue *q){
	q->head = NULL;
	q->empty = 1;
	q->request_exit = 0;

	pthread_mutex_init(&q->lock, NULL);
	pthread_cond_init(&q->dequeue_ready, NULL);

	return q;
}

char *dequeue(queue *q, int *thread_wait_count){
	pthread_mutex_lock(&q->lock);

	while(q->empty){
		(*thread_wait_count)++; 
		pthread_cond_wait(&q->dequeue_ready, &q->lock);	
		(*thread_wait_count)--; 

		if(q->request_exit){
			pthread_mutex_unlock(&q->lock);
			return NULL;
		}
	}

	node *h = q->head;
	char *ret = h->data;	
	q->head = h->next;
	free(h);

	if(q->head == NULL)
		q->empty = 1;	

	pthread_mutex_unlock(&q->lock);
	return ret;
}

char *make_string(char *data){
	int len = strlen(data);
	char *ret = malloc(len + 1);

	memcpy(ret, data, len);
	ret[len] = '\0'; 
	return ret;
}

void enqueue(queue *q, char *data){
	pthread_mutex_lock(&q->lock);

	if(q->empty)	
		q->empty = 0;

	char *eq_data = make_string(data); 
	node *eq = malloc(sizeof(node));
	eq->data = eq_data; 	
	eq->next = q->head;
	q->head = eq;

	pthread_cond_signal(&q->dequeue_ready);
	pthread_mutex_unlock(&q->lock);
} 

int is_empty(queue *q){
	pthread_mutex_lock(&q->lock);

	int ret = 0;
	if(q->empty)
		ret = 1;		

	pthread_mutex_unlock(&q->lock);
	return ret;
}

void request_exit(queue *q, int thread_count){
	pthread_mutex_lock(&q->lock);

	q->request_exit = 1;
	int i = thread_count;
	while(i-- > 0)
		pthread_cond_signal(&q->dequeue_ready);		

	pthread_mutex_unlock(&q->lock);
}
