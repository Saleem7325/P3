#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>

/*--- -r ----*/
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include "DS/directory_stack.h"
#include "DS/file_stack.h"
#include "word_wrap/word_wrap.h"

int dir_threads = 1;
int file_threads = 1;

directory_stack *dir_s;
file_stack *file_s;

int line_size;

int recursive = 1;
int file_arg = 0;

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

int valid_file_name(char *name){
	if(name[0] == '.')
		return 0;
	
	if(strlen(name) >= 5 && strncmp("wrap.", name, 5) == 0) 
		return 0;
	
	return 1;
}

int valid_line_size(char *str){
	int val = atoi(str);  

	if(digits_only(str) && val > 0){
		line_size = val /*+ 1*/;
		return 1;
	}else
		return 0;
}

/*
* Checks whether a file is a directory
*/
int is_directory(char *path){
	struct stat sbuf;
	stat(path, &sbuf);
	return S_ISDIR(sbuf.st_mode);	
}

/*
* Checks whether a file is a regular file
*/
int is_reg_file(char *path){
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

//int dir_wait_count = 0;
//int file_wait_count = 0;


void read_directory(char *dir_name){
	DIR *directory = opendir(dir_name);
	if(directory == NULL){
		free(dir_name);
		closedir(directory);
		return;
	}

	struct dirent *entry = readdir(directory);
	while(entry != NULL){
		char *name = entry->d_name;
		char *path = get_path(dir_name, name);
		
		if(is_reg_file(path) && valid_file_name(name))
			push_f(file_s, path);
		else if(recursive && is_directory(path) && name[0] != '.')	
			push_d(dir_s, path);	
		
		free(path);
		entry = readdir(directory);
	}	

	free(dir_name);
	closedir(directory);
}

/*int dir_cond(){
	return (dir_threads == 1 && !is_empty(dir_q)) || (!is_empty(dir_q) && dir_wait_count != (dir_threads - 1)); 
}*/

void *directory_work(void *arg){
	char *dir_name = pop_d(dir_s);
	while(dir_name != NULL/*dir_cond()*/){
		//char *dir_name = dequeue(directory_q, &dir_wait_count);
		//if(dir_name == NULL)
		//	pthread_exit(arg);
		//else
		read_directory(dir_name);	

		if(!recursive)
			break;	

		dir_name = pop_d(dir_s);
	}	

	//request_exit(directory_q, dir_wait_count);
	close_file_stack(file_s);
	pthread_exit(arg);
}

void *file_work(void *arg){
	word_wrap *ww = malloc(sizeof(word_wrap));	
	init_word_wrap(ww, line_size);

	char *file_name = pop_f(file_s);
	while(file_name != NULL){
		write_file(ww, file_name, file_arg);
		free(file_name);
		file_name = pop_f(file_s);

	}	

	free(ww);
	pthread_exit(arg);
}

void free_all(){
	free(file_threads_tid);
	free(file_args);
	free(file_s);

	if(!file_arg){
		free(dir_s);
		free(dir_threads_tid);
		free(dir_args);
	}
}

void join_threads(){
	if(!file_arg){
		for(int i = 0; i < dir_threads; i++){
			pthread_join(dir_threads_tid[i], NULL);	
			printf("Dir thread %d returns: %d\n", i, dir_args[i]); 
		}
	}

	//while(file_wait_count != file_threads)
		//usleep(100);

	//request_exit(file_q, file_threads);

	for(int i = 0; i < file_threads; i++){
		pthread_join(file_threads_tid[i], NULL);
		printf("File thread %d returns: %d\n", i, file_args[i]); 
	}
	
	free_all();
}

void start_threads(){
	if(!file_arg){
		dir_threads_tid = malloc(sizeof(pthread_t) * dir_threads);
		dir_args = malloc(sizeof(int) * dir_threads);

		for(int i = 0; i < dir_threads; i++){
			dir_args[i] = 1;
			pthread_create(&dir_threads_tid[i], NULL, directory_work, &dir_args[i]);	
		}	
	}

	file_threads_tid = malloc(sizeof(pthread_t) * file_threads);
	file_args = malloc(sizeof(int) * file_threads);

	for(int i = 0; i < file_threads; i++){
		file_args[i] = 1;
		pthread_create(&file_threads_tid[i], NULL, file_work, &file_args[i]);
	}

	join_threads();
}

void init_mem(){
	file_s = malloc(sizeof(file_stack));	 
	init_file_stack(file_s, file_threads);

	if(file_arg)
		return;

	dir_s = malloc(sizeof(directory_stack));	
	init_directory_stack(dir_s, dir_threads);
}

void file_arg_case(char *file){
	file_arg = 1;
	init_mem();
	push_f(file_s, file);	
	close_file_stack(file_s);
	//start_threads();
}

void dir_arg_case(char *dir){
	//file_threads = 5;
	init_mem();
	push_d(dir_s, dir);	
	//start_threads();
}
		
int main(int argc, char **argv){
	if(argc < 2)
		exit_perror(1);

	if(argc == 2 && valid_line_size(argv[1])){

		file_arg_case("STDIN_FILENO");
//		puts("digits only 2 args\n");

	}else if(argc == 3 && valid_line_size(argv[1])){

		if(is_directory(argv[2])){
			recursive = 0;
			dir_arg_case(argv[2]);
		//	init_mem();
		//	enqueue(directory_q, argv[3]);
		//	start_threads();

		}else if(is_reg_file(argv[2]) && valid_file_name(argv[2])){

			file_arg_case(argv[2]);

		}else
			exit_perror(2);

//		puts("digits only 3 args\n");

	}else if(argc == 4 && valid_first_arg(argv[1]) && valid_line_size(argv[2]) && is_directory(argv[3])){

//		printf("4 args/valid first arg\nM:%d\nN:%d\n", dir_threads, file_threads);	
		dir_arg_case(argv[3]);
		//init_mem();
		//enqueue(directory_q, argv[3]);
		//start_threads();
	}else
		exit_perror(1);

	start_threads();

	exit(EXIT_SUCCESS);

}




