#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
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

pthread_t *dir_threads_tid;
pthread_t *file_threads_tid;

/*----------------------------------------------------------------- Input validation functions -------------------------------------------------------*/

/*
* Recieves a null terminated char *, returns 1 if string contains only digits or is of length 0 otherwise returns 0.
*/
int digits_only(char *str){
	int len = strlen(str);

	for(int i = 0; i < len; i++)
		if(!isdigit(str[i]))
			return 0;	
	return 1;
}

/*
* Recieves a null terminated char *str, length of char *str, and index into char *str, and returns the int value
* of the substring str[0]..str[index - 1] if the substring contains only digits otherwise returns 0. 
*/
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

/*
* Recieves a null terminated char *str, length of char *str, and index into char *str, and returns the int value
* of the substring str[index + 1]..str[len - 1] if the substring contains only digits otherwise returns 0. 
*/
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

/*
* Recieves a null terminated char *str, chekcs whether the first two charcters in str are equal to "-r"
*/
int valid_prefix(char *str){
	char prefix[3];
	prefix[2] = '\0';
	strncpy(prefix, str, 2);

	if(strcmp(prefix, "-r") != 0)
		return 0;
	return 1;
}


/*
* Recieves a null terminated char *str, and the length of str. Checks whether str is of the following formats:
*
*	1) "-r"
*	2) "-rN" where N is a postive interger
*	3) "-rM,N" where N and M are both positive integers
*	 
* If the characters in str do not math any of the previous formats specified, valid_suffix returns 0, otherwise returns 1.
*/
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

/*
* Recieves a null terminated char *str, and returns 1 if str is a valid first argument, otherwise returns 0.
*/
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

/*
* Recieves a null terminated char * parent and child, returns a null terminated char *path which is parent + "/" + child 
*/
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

/*
* Recieves a null terminated char *name, returns 0 if first character is '.' or if the first characters are "wrap."
* otherwise returns 1
*/
int valid_file_name(char *name){
	if(name[0] == '.')
		return 0;
	
	if(strlen(name) >= 5 && strncmp("wrap.", name, 5) == 0) 
		return 0;
	
	return 1;
}

/*
* Recieves a null terminated char *name, returns 1 if str contains only digits and the integer value of str is greater than 1
* otherwise returns o.  
*/
int valid_line_size(char *str){
	int val = atoi(str);  

	if(digits_only(str) && val > 0){
		line_size = val;
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

/*-------------------------------------------------------------------------- Thread functions -------------------------------------------------------------------- */

/*
* Recieves a null terminated char *dir_name corresponding to a directory path, opens the directory and reads every directory entry
* if the entry is a valid directory the corresponding directory path is pused to dir_s, otherwise if the entry is a valid regular file
* the corresponding file path is pushed onto the file_s 
*/
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

/*
* Pops from dir_s, if the entry is not null, the valid entries in the corresponding directory are pushed onto the relevant stack. 
* Depending on which flags are set this process may repeatedly execute until NULL is received from pop_d.
*/
void *directory_work(void *arg){
	char *dir_name = pop_d(dir_s);
	while(dir_name != NULL){
		read_directory(dir_name);	

		if(!recursive)
			break;	

		dir_name = pop_d(dir_s);
	}	

	close_file_stack(file_s);
	return NULL;
}

/*
* Repeatedly pops file_s and wraps each file path recieved from pop_f until NULL is recieved from pop_f. 
*/
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
	return NULL;
}


/*-------------------------------------------------------------- Main thread functions ------------------------------------------------------------------------*/

/*
* Recieves an int, prints correspondning error message.
*/
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
		default:
			return;	
	}
	exit(EXIT_FAILURE);	
}

/*
* Frees all memory allocated by ww.
*/
void free_all(){
	free(file_threads_tid);
	free(file_s);

	if(!file_arg){
		free(dir_s);
		free(dir_threads_tid);
	}
}

/*
* Joins every thread started, and frees all memory allocated by ww.
*/
void join_threads(){
	if(!file_arg){
		for(int i = 0; i < dir_threads; i++)
			pthread_join(dir_threads_tid[i], NULL);	
	}

	for(int i = 0; i < file_threads; i++)
		pthread_join(file_threads_tid[i], NULL);
	
	free_all();
}

/*
* Starts threads and allocates necessary resourses.
*/
void start_threads(){
	if(!file_arg){
		dir_threads_tid = malloc(sizeof(pthread_t) * dir_threads);

		for(int i = 0; i < dir_threads; i++)
			pthread_create(&dir_threads_tid[i], NULL, directory_work, NULL);	
			
	}

	file_threads_tid = malloc(sizeof(pthread_t) * file_threads);

	for(int i = 0; i < file_threads; i++)
		pthread_create(&file_threads_tid[i], NULL, file_work, NULL);
	

	join_threads();
}

/*
* Initilizes file_s and/or dir_s depending on which flags are set.
*/
void init_mem(){
	file_s = malloc(sizeof(file_stack));	 
	init_file_stack(file_s, file_threads);

	if(file_arg)
		return;

	dir_s = malloc(sizeof(directory_stack));	
	init_directory_stack(dir_s, dir_threads);
}

/*
* Receives a null terminated char *file, sets file_arg flag, initializes relevant stacks, pushes file onto file_s, and closes file_s pool.
*/
void file_arg_case(char *file){
	file_arg = 1;
	init_mem();
	push_f(file_s, file);	
	close_file_stack(file_s);
}

/*
* Receives a null terminated char *dir, initializes relevant stacks, pushes dir onto dir_s.
*/
void dir_arg_case(char *dir){
	init_mem();
	push_d(dir_s, dir);	
}

/*
* Determines which flags to set and resourses to allocate depending on user input, and starts threads if input is valid otherwise prints error message.
*/		
int main(int argc, char **argv){
	if(argc < 2)
		exit_perror(1);
	if(argc == 2 && valid_line_size(argv[1]))
		file_arg_case("STDIN_FILENO");
	else if(argc == 3 && valid_line_size(argv[1])){
		if(is_directory(argv[2])){
			recursive = 0;
			dir_arg_case(argv[2]);
		}else if(is_reg_file(argv[2]) && valid_file_name(argv[2]))
			file_arg_case(argv[2]);
		else
			exit_perror(2);
	}else if(argc == 4 && valid_first_arg(argv[1]) && valid_line_size(argv[2]) && is_directory(argv[3]))
		dir_arg_case(argv[3]);
	else
		exit_perror(1);

	start_threads();
	exit(EXIT_SUCCESS);
}




