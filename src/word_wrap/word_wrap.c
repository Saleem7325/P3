/*#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#define MODE S_IRUSR|S_IWUSR
#define FFLAGS O_RDONLY 
#define WRITE_FLAGS O_CREAT|O_WRONLY|O_APPEND|O_TRUNC
#define BUFSIZE 1024*/ 
#include "word_wrap.h"

const char white_chars[6] = {'\t', '\n', ' ', '\v', '\f', '\r'};

/*------------------------------------ initialize lock/attributes -------------------------*/

word_wrap *init_word_wrap(word_wrap *ww, int size){
	ww->dyn_buf = NULL;	
	ww->total = 0;
	ww->line_size = size; 
	ww->total = 0;
	ww->bytes_read = 0;
	ww->exit_code = 0;
	ww->line_start = 0;
	return ww;
}

/*------------------------------------- Write File Helper functions --------------------------*/

/*
* Takes a char * that contains the file name of the file we want to wrap
* returns a char * containing with wrap. added before name
*/
char *get_file_name(char *name){
	int reg_file_name_length = strlen(name) + 1;
	char prefix[] = "wrap.";
	char *filename = malloc(reg_file_name_length + 5);
	
	for(int i = 0; i < 5; i++)
		filename[i] = prefix[i];
	
	int pos = 5;
	for(int i = 0; i < reg_file_name_length; i++, pos++)
		filename[pos] = name[i];

	return filename;
}

char *get_file_path(char *path){
	int len = strlen(path);
	int i = len - 1;
	while(i > -1 && path[i] != '/')
		i--;

	if(i > -1){
		//Separate file name from path and get the proper output/wrap file name
		char *file_name = malloc(len - i);
		file_name[len - i - 1] = '\0';
		strncpy(file_name, path + i + 1, len - i - 1); 			
		char *out_file_name = get_file_name(file_name);	
		//printf("%s\n", file_name);
		//printf("%s\n", out_file_name);
		//Create path for the output file from the input file path	
		char *out_file_path = malloc(strlen(out_file_name) + i + 2);
		out_file_path[strlen(out_file_name) + i + 1] = '\0';
		strncpy(out_file_path, path, i + 1); 			
		strncpy(out_file_path + i + 1, out_file_name, strlen(out_file_name)); 			
		free(file_name);
		free(out_file_name);	
		//printf("%s\n", out_file_path); 
		return out_file_path;
	}else
		return get_file_name(path);
			
}

int set_file_descriptors(word_wrap *ww, char *path, int file_arg){
	if(strcmp(path, "STDIN_FILENO") == 0){
		ww->ifd = STDIN_FILENO;
		ww->ofd = STDOUT_FILENO; 
		return 0;
	}

	ww->ifd = open(path, READ_FLAGS);
	if(ww->ifd < 0){
		ww->exit_code = 2; //set proper exit code
		return 1;
	}else if(file_arg){
		ww->ofd = STDOUT_FILENO; 
		return 0;		
	}
		
	char *file_path = get_file_path(path);
	ww->ofd = open(file_path, WRITE_FLAGS, MODE);
	free(file_path);

	if(ww->ofd < 0){
		ww->exit_code = 2; //set proper exit code
		return 1;
	}	
	
	return 0;
}

/*--------------------------------------- Normalize helper functions ----------------------------------*/

/*
* Prints a new line, prob best if this gets deleted.
*/
void newLine(word_wrap *ww){
	char p = '\n';
	write(ww->ofd, &p, 1);
}

/*
* Checks if character passed is a white space char
*/
int is_white_char(char c){
	for(int i = 0; i < 6; i++)
		if(white_chars[i] == c)
			return 1;
	return 0;
}

/*
* If the last char in line is not a white space, given an index p findStart will
* find the first index that is a white space starting at p and decrementing until
* a whitespace is reached. If p reaches 0 without encountering a white space it will
* return 0. 
*/
int findStart(word_wrap *ww, int p){
	int start = p;
	while(start > -1 && (!is_white_char(ww->dyn_buf[start]))) //changed to -1 from 0
		start--;

	if(start < 0)
		return 0;
	return start;
}

/*
* Pretty much the same thing at findStart but p gets incremented instead, typically used when
* a word is too big to fit on a line and we need to collect the rest of the chars from another 
* read call.
*/
int findEnd(word_wrap *ww, int p){
	int end = p;
	while(end < ww->total && (!is_white_char(ww->dyn_buf[end])) )
		end++;

	if((end + 1) < ww->total && ww->dyn_buf[end] == '\n' && ww->dyn_buf[end + 1] == '\n')
		return ++end;
	return end;
}


/*
* Copies from size bytes from buffer to cutoff starting at index in buffer
*/
void copy_to_dyn_buf(word_wrap * ww){
	int index = ww->total;
	for(int i = 0; i < ww->bytes_read; i++, index++)
		ww->dyn_buf[index] = ww->buf[i];
	 
}

int first_char_white(word_wrap * ww, int i){
	char curr = ww->dyn_buf[i];	

	if(is_white_char(curr)){
		int p = i;
		while(p < ww->total && is_white_char(ww->dyn_buf[p]))
			p++;

		if(p >= ww->total){
			if(i == 0)
				ww->total = 0;
			else
				ww->total = i /*- 1*/;
						
			return 0;
		}
		memmove(ww->dyn_buf + i, ww->dyn_buf + p, ww->total - p);
		ww->total = ww->total - (p - i); //was - (p - i)
	}
	return 1;
}

void shift_left(word_wrap * ww, int *i){
	memmove(ww->dyn_buf + *i - 1, ww->dyn_buf + *i, ww->total - *i);
	ww->total--;	
	(*i)--;
}

int last_char_in_line(word_wrap * ww, int *i, int *space_count, int *newline_count, int *first_char, int *size){
	char curr = ww->dyn_buf[*i];	

	if(curr == '\n'){
		if(*space_count == 1){
			shift_left(ww, i);
			*space_count = 0;
		}else if (*newline_count == 1){
			*newline_count = 0;
			*first_char = 1;
		}
		*size = 0;	
		*first_char = 1;
		return 1;
			
	}else if(is_white_char(curr)){
		if(*space_count == 1){
			shift_left(ww, i);	
			*space_count = 0;
			ww->dyn_buf[*i] = '\n';		
		}else if(*newline_count == 1){
			(*i)--;
			*newline_count = 0;
		}else{
			ww->dyn_buf[*i] = '\n';
		}
		*size = 0;
		*first_char = 1;
		return 1;
	}else{
		int start = findStart(ww, *i);
		int end = findEnd(ww, *i);

		if(start > ww->line_start){
			ww->dyn_buf[start] = '\n';
			*i = start;
			*size = 0;
			*first_char = 1;
		}else if (end < ww->total){
			ww->dyn_buf[end] = '\n';
			*i = end;
			*size = 0;
			*first_char = 1;	
			ww->exit_code= 2;
		}else if (end >= ww->total && start <= ww->line_start){
			ww->exit_code= 2;
			return 0;
		}
	}
	return 1;
}

void not_last_char_in_line(word_wrap *ww, int *i, int *space_count, int *newline_count, int *first_char, int *size){
	char curr = ww->dyn_buf[*i];	

	if(is_white_char(curr)){
		if(curr == '\n'){
			if(*newline_count == 0){
				*newline_count = 1;
				(*size)++;	
			}else if(*newline_count == 1){
				*newline_count = 0;
				*space_count = 0;	
				*size = 0;
				*first_char = 1;
			}
			if(*space_count == 1){
				shift_left(ww, i);
				(*size)--;//new
				*space_count = 0;
				*newline_count = 1;
			}

		}else if(curr == ' '){
			if(*space_count == 1 || *newline_count == 1){
				shift_left(ww, i);
				*newline_count = 0;
				*space_count = 1;
			}else{ //new else
				*space_count = 1;
				(*size)++;
			}
		}else{
			if(*space_count == 1){
				shift_left(ww, i);
				*space_count = 1;
				ww->dyn_buf[*i] = ' ';
			}else if (*newline_count == 1){
				shift_left(ww, i);
				*newline_count = 1;
				ww->dyn_buf[*i] = '\n';
			}else{
				ww->dyn_buf[*i] = ' ';	
				*space_count = 1;
			}
		}	
	}else{
		if(*newline_count == 1){
			ww->dyn_buf[*i-1] = ' ';
		}
		(*size)++;
		*newline_count = 0;
		*space_count = 0;	
	}
}

void normalize(word_wrap *ww){
	ww->dyn_buf = realloc(ww->dyn_buf, ww->total + ww->bytes_read);
	if(ww->dyn_buf == NULL){
		perror("Memory allocation error");
		puts("asdfsadf");
		return;
	}

	copy_to_dyn_buf(ww);
	ww->total += ww->bytes_read;

	int first_char = 1;
	int space_count = 0;
	int newline_count = 0;
	int size = 0;

	for(int i = ww->line_start; i < ww->total; i++){
		if(first_char){
			if(is_white_char(ww->dyn_buf[i])){
				int fc = first_char_white(ww, i);	
				if(fc == 0)
					break;
			}

			ww->line_start = i;
			first_char = 0;	
			size++;
			newline_count = 0;
			space_count = 0;
			continue;
		}

		if(size == ww->line_size){
			int lc = last_char_in_line(ww, &i, &space_count, &newline_count, &first_char, &size);
			if(lc)
				continue;
			break;
		}else{ // not last char in line case
			not_last_char_in_line(ww, &i, &space_count, &newline_count, &first_char, &size);
		}
	}
}

/*----------------------- Write File ----------------------*/

/*
* The goal of this function is to read all bytes from file, normalize the buffer as bytes
* are read, then print the buffer after the chars have been normalized.
*/
void write_file(word_wrap *ww, char *path, int file_arg){
	if(set_file_descriptors(ww, path, file_arg))
		return;

	//reads from file if bytes_read > 0 it continues to read from file until bytes_read < 1	
	do{	
		ww->bytes_read = read(ww->ifd, ww->buf, BUFSIZE);
		if(ww->bytes_read <= 0)
			break;
		
		normalize(ww);
	}while(ww->bytes_read > 0);

	if(ww->total > 2 && ww->dyn_buf[ww->total - 1] == '\n' && ww->dyn_buf[ww->total - 2] == '\n') 
		ww->total--;

	write(ww->ofd, ww->dyn_buf, ww->total);
	if(ww->total > 0 && ww->dyn_buf[ww->total - 1] != '\n')
		newLine(ww);

	free(ww->dyn_buf);
	ww->dyn_buf = NULL;
	ww->total = 0;
	ww->line_start = 0;
}

