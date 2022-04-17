#include <stdio.h>
//#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>

/*--- -r ----*/
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include "DS/queue.h"

int dir_threads = 1;
int file_threads = 1;

queue *directory_q;
queue *file_q;

/*-------------------------- Input validation functions -----------------------------*/

int digits_only(char *str){
	int len = strlen(str);

	for(int i = 0; i < len; i++)
		if(!isdigit(str[i]))
			return 0;	
	return 1;
}

int getM(char *str, int len, int index){
	char *m = malloc(index + 1);
	m[index] = '\0'; 
	strncpy(m, str, index);	

	if(!digits_only(m)){
		free(m);
		return 0;
	}	
	
	int ret = atoi(m);
	free(m);
	return ret;
}

int getN(char *str, int len, int index){
	char *n = malloc(len - index);
	strncpy(n, str + index + 1, len - index);	

	if(!digits_only(n)){
		free(n);
		return 0;	
	}

	int ret = atoi(n);
	free(n);
	return ret;
}

int valid_prefix(char *str){
	char prefix[3];
	prefix[2] = '\0';
	strncpy(prefix, str, 2);

	if(strcmp(prefix, "-r") != 0)
		return 0;
	return 1;
}

int valid_suffix(char *str, int len){
	if(len == 2)
		return 1;

	int ret = 0;
	char *suffix = malloc(len - 1);
	suffix[len - 2] = '\0';
	strncpy(suffix, str + 2, len - 2);	
	
	if(digits_only(suffix)){
		int n = atoi(suffix);

		if(n > 0){
			file_threads = n;
			ret = 1;
		} 
			free(suffix);
			return ret;
	}	

	int suf_len = strlen(suffix);	

	for(int i = 0; i < suf_len; i++){
		if(!isdigit(suffix[i])){
			if(i < 1 || suffix[i] != ',')
				break;

			file_threads = getN(suffix, suf_len, i);
			dir_threads = getM(suffix, suf_len, i);

			if(file_threads > 0 && dir_threads > 0)
				ret = 1;
			break;			
		}
	}	

	free(suffix);
	return ret;
}

int valid_first_arg(char *str){
	int len = strlen(str);
	if(len < 2)
		return 0; 
	
	if(!valid_prefix(str))
		return 0;

	if(!valid_suffix(str, len))	
		return 0;

	return 1;
}

int valid_line_size(char *str){
	if(digits_only(str) && atoi(str) > 0)
		return 1;
	return 0;
}

/*
* Checks whether a file is a directory
*/
int isDirectory(char *path){
	struct stat sbuf;
	stat(path, &sbuf);
	return S_ISDIR(sbuf.st_mode);	
}

/*
* Checks whether a file is a regular file
*/
int isRegularFile(char *path){
	struct stat sbuf;
	stat(path, &sbuf);
	return S_ISREG(sbuf.st_mode);
}

/*--------------------------- Exit error functions -----------------*/

void exit_perror(int code){
	switch(code){
		case 1 : 
			errno = EINVAL; 
			perror("Erorr");
			break;
		case 2: 
			errno = EPERM; 
			perror("Unreadable input file");
			break;  
		case 3:
			errno = EPERM; 
			perror("Unable to create output file");
			break;
		case 4:
			errno = EINVAL; 
			perror("Word longer than line width");
			return;
		default:
			puts("Invalid error code\n");
			return;	
	}
	exit(EXIT_FAILURE);	
}

/*----------------------- -r scenario ------------- */

pthread_t *dir_threads_tid;
pthread_t *file_threads_tid;
int *dir_args;
int *file_args;
//int wait_count = 0;

int dir_wait_count = 0;
int file_wait_count = 0;

int dir_work_done = 0;
//pthread_barrier_t *barrier;

char *get_path(char *parent, char *child){
	int plen = strlen(parent);
	int clen = strlen(child);
	int size = plen + clen + 2;

	char *path = malloc(size);
	strncpy(path, parent, plen);

	path[plen] = '/';
	strncpy(path + plen + 1, child, clen);
	path[size - 1] = '\0';

	return path;
}

void *directory_work(void *arg){
	while(/*1*/(dir_threads == 1 && !is_empty(directory_q)) || (!is_empty(directory_q) && dir_wait_count != (dir_threads - 1)) ){
		dir_wait_count++;
		char *dir_name = dequeue(directory_q);
		dir_wait_count--;

		if(dir_name == NULL)
			pthread_exit(arg);
			//return arg;

		DIR *directory = opendir(dir_name);

		if(directory == NULL){
			//puts("Directory == NULL");	
			free(dir_name);
			continue;
		}

		struct dirent *entry = readdir(directory);
		while(entry != NULL){
			char *name = entry->d_name;

			if(isRegularFile(name) && name[0] != '.' /*&& strncmp("wrap.", name, 5) != 0*/){
				//printf("%s\n", name);
				char *path = get_path(dir_name, name);
				enqueue(file_q, path);
				free(path);
			}else if(isDirectory(name) && name[0] != '.'){ 	
				char *path = get_path(dir_name, name);
				printf("%s\n", name);
				enqueue(directory_q, path);
				free(path);
			}
				
			//if(name[0] != '.') printf("%s\n", name);

			entry = readdir(directory);
		}	
		closedir(directory);
		free(dir_name);
	}	
	request_exit(directory_q, dir_threads - 1);
	dir_work_done = 1;
	pthread_exit(arg);
	//return arg;
}

void *file_work(void *arg){
	while((file_threads == 1 && !is_empty(file_q) && !dir_work_done) || (!dir_work_done && !is_empty(file_q) && file_wait_count != (file_threads - 1)) ){
		file_wait_count++;
		char *file_name = dequeue(file_q);
		file_wait_count--;

		if(file_name == NULL)
			pthread_exit(arg);

		printf("%s\n", file_name);
		free(file_name);
	}	
	request_exit(file_q, file_threads - 1);
	pthread_exit(arg);
}

/*
void start_file_threads(){
	pthread_barrier_init(&barrier, NULL, file_threads + 1);	

	for(int i = 0; i < file_threads; i++)
		pthread_create(&file_threads_tid[i], NULL, file_work, NULL);
	while(
	
	
}
*/

void wait_for_threads(){
	/*while(wait_count != (file_threads + dir_threads)){
		usleep(100);
	}
	
	for(int i = 0; i < dir_threads; i++){
		pthread_cancel(dir_threads_tid[i]);	
		printf("Dir thread %d returns: %d\n", i, dir_args[i]); 
	}

	for(int i = 0; i < file_threads; i++){
		pthread_cancel(file_threads_tid[i]);
		printf("File thread %d returns: %d\n", i, file_args[i]); 
	}*/	

	for(int i = 0; i < dir_threads; i++){
		pthread_join(dir_threads_tid[i], NULL);	
		printf("Dir thread %d returns: %d\n", i, dir_args[i]); 
	}

	for(int i = 0; i < file_threads; i++){
		pthread_join(file_threads_tid[i], NULL);
		printf("File thread %d returns: %d\n", i, file_args[i]); 
	}

	free(file_threads_tid);
	free(dir_threads_tid);
	free(dir_args);
	free(file_args);	
}

void start_threads(){
	dir_threads_tid = malloc(sizeof(pthread_t) * dir_threads);
	file_threads_tid = malloc(sizeof(pthread_t) * file_threads);

	dir_args = malloc(sizeof(int) * dir_threads);
	file_args = malloc(sizeof(int) * file_threads);

	for(int i = 0; i < dir_threads; i++){
	//	dir_args[i] = malloc(sizeof(int));
		dir_args[i] = 1;
		pthread_create(&dir_threads_tid[i], NULL, directory_work, &dir_args[i]/*NULL*/);	
	}

	for(int i = 0; i < file_threads; i++){
		//file_args[i] = malloc(sizeof(int));
		file_args[i] = 1;
		pthread_create(&file_threads_tid[i], NULL, file_work, &file_args[i]/*NULL*/);
	}
	wait_for_threads();
}

void join_threads(){
	for(int i = 0; i < dir_threads; i++)
		pthread_join(dir_threads_tid[i], NULL);	

	for(int i = 0; i < file_threads; i++)
		pthread_join(file_threads_tid[i], NULL);
	
	free(file_threads_tid);
	free(dir_threads_tid);
}
		
int main(int argc, char **argv){
	if(argc < 2)
		exit_perror(1);

	if(argc == 2 && valid_line_size(argv[1])){
		
		puts("digits only 2 args\n");

	}else if(argc == 3 && valid_line_size(argv[1])){

		puts("digits only 3 args\n");

	}else if(argc == 4 && valid_first_arg(argv[1]) && valid_line_size(argv[2]) && isDirectory(argv[3])){
		printf("4 args/valid first arg\nM:%d\nN:%d\n", dir_threads, file_threads);	
		file_q = malloc(sizeof(queue));	 
		init_queue(file_q);

		directory_q = malloc(sizeof(queue));	
		init_queue(directory_q);

		enqueue(directory_q, argv[3]);

		start_threads();
		//join_threads();

	}else
		exit_perror(1);

	free(directory_q);
	free(file_q);
	exit(EXIT_SUCCESS);

}




