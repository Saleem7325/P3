#include "stdio.h"
#include "queue.h"

char words[20][10] = { "Word", "1", "3", "Random", "*923", "6", "7", "8", "sdfs", "10", "11", "12", "strngs", "14", "15", "sdfssdd", "17", "18", "sasuke", "20"};
queue *q;
pthread_t tid[10];
pthread_t tidd[10];

void *enqueue_q(void *arg){
	for(int i = 0; i < 3; i++)
		enqueue(q, words[rand() % 20]);		
	return NULL;
}

void *dequeue_q(void *arg){
	while(!is_empty(q)){
		char *d = dequeue(q);
		printf("%s\n", d);
		free(d);
	}
	return NULL;
}

void print_queue(queue *q){
	if(q->empty){
		puts("Queue is empty");
		return;
	}

	node *h = q->head;
	while(h != NULL){
		node *n = h->next;
		printf("%s\n", h->data);
		free(h->data);
		free(h);
		h = n;
	}	
}

void start_threads(){
	for(int i = 0; i < 10; i++)
		pthread_create(&tid[i], NULL, enqueue_q, NULL);

	for(int i = 0; i < 10; i++)
		pthread_create(&tidd[i], NULL, dequeue_q, NULL);
}

void join_threads(){
	for(int i = 0; i < 10; i++)
		pthread_join(tid[i], NULL);

	for(int i = 0; i < 10; i++)
		pthread_join(tidd[i], NULL);
}

int main(int argc, char **argv){
	q = malloc(sizeof(queue));
	init_queue(q);
	
	start_threads();
	join_threads();
	free(q);
	return EXIT_SUCCESS;			
}
