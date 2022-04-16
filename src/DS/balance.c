#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h> 

typedef struct node{
	char *data;
	struct node *next;
}node;

node *head = NULL;

void push(char str){
	char *d = malloc(sizeof(char));
	*d = str;

	if(head == NULL){
		head = malloc(sizeof(node));
		head->data = d;
		head->next = NULL;
	}else{
		node *list = malloc(sizeof(node));
		list->data = d;
		list->next = head;
		head = list;	
	}	
}

char pop(){
	if(head == NULL){
		return 'v';
	}else{
		node *p = head->next;
		char d;
		d = *(head->data);
		free(head->data);
		free(head);
		head = p;
		return d;
	}
}

int isEmpty(){
	if(head == NULL){ return 1; }else{ return 0;}
}

int main(int argc, char **argv){
	if((argc == 1) || (strlen(argv[1]) == 0)){
		exit(EXIT_SUCCESS);
	}

	for(int i = 0; i < strlen(argv[1]); i++){
		switch(argv[1][i]){
			case '(' : push(argv[1][i]); break;
			case '[' : push(argv[1][i]); break;
			case '{' : push(argv[1][i]); break;
			case ')' : if(pop() != '('){ printf("%d: %c\n", i , argv[1][i]); exit(EXIT_FAILURE); } break;
			case ']' : if(pop() != '['){ printf("%d: %c\n", i , argv[1][i]); exit(EXIT_FAILURE); } break;
			case '}' : if(pop() != '{'){ printf("%d: %c\n", i , argv[1][i]); exit(EXIT_FAILURE); } break;
		}
	}

	if(isEmpty()){
		exit(EXIT_SUCCESS);
	}else{
		char d;
		printf("%s", "open: ");
		while(isEmpty() == 0){
			d = pop();
			switch(d){
				case '(' : printf("%c", ')'); break;
				case '[' : printf("%c", ']'); break;
				case '{' : printf("%c", '}'); break;
			}	
		}
		printf("%c", '\n');
		exit(EXIT_FAILURE);
	}	

	
}

