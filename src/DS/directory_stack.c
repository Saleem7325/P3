#include "directory_stack.h"

/*
* Sets attributes to reflect an empty stack and initializes lock/pop_read condition
*/
directory_stack *init_directory_stack(directory_stack *s, int thread_count){
	s->head = NULL;
	s->empty = 1;
	s->closed = 0;
	s->threads = thread_count;
	s->active_threads = thread_count;

	pthread_mutex_init(&s->lock, NULL);
	pthread_cond_init(&s->pop_ready, NULL);

	return s;
}

char *pop_d(directory_stack *s){
	pthread_mutex_lock(&s->lock);

	while(s->empty){
		if(s->active_threads == 1){
			close_directory_stack(s);
			return NULL;
		}

		s->active_threads--; 
		pthread_cond_wait(&s->pop_ready, &s->lock);	
		s->active_threads++; 

		if(s->closed){
			pthread_mutex_unlock(&s->lock);
			return NULL;
		}
	}

	node *h = s->head;
	char *ret = h->data;	
	s->head = h->next;
	free(h);

	if(s->head == NULL)
		s->empty = 1;	

	pthread_mutex_unlock(&s->lock);
	return ret;
}

/*
* Recieves a char *data. Called by push_d to dynamically allocate enough bytes to store the value needed to be pushed.
*/
char *make_string_d(char *data){
	int len = strlen(data);
	char *ret = malloc(len + 1);

	memcpy(ret, data, len);
	ret[len] = '\0'; 
	return ret;
}

void push_d(directory_stack *s, char *data){
	pthread_mutex_lock(&s->lock);

	if(s->empty)	
		s->empty = 0;

	char *eq_data = make_string_d(data); 
	node *eq = malloc(sizeof(node));
	eq->data = eq_data; 	
	eq->next = s->head;
	s->head = eq;

	pthread_cond_signal(&s->pop_ready);
	pthread_mutex_unlock(&s->lock);
} 

/*
* Sets s->closed = 1, and wakes up any threads waiting to pop in s..
*/
void close_directory_stack(directory_stack *s){
	s->closed = 1;

	int i = s->active_threads;
	while(i++ < s->threads)
		pthread_cond_signal(&s->pop_ready);		

	pthread_mutex_unlock(&s->lock);
}
