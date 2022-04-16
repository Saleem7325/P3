#include <stdlib.h>
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
#define BUFSIZE 1 

char *buf;
int buf_size = BUFSIZE;
char *dyn_buf = NULL;
int total = 0;
int line_size; 
int fd; 
int ofd; 
int bytes_read = 0;
char white_chars[6] = {'\t', '\n', ' ', '\v', '\f', '\r'};
int fail = 0;

/*
* Prints a new line, prob best if this gets deleted.
*/
void newLine(int ofd){
	char p = '\n';
	write(ofd, &p, 1);
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
int findStart(int p){
	int start = p;
	while(start > -1 && (!is_white_char(dyn_buf[start]))) //changed to -1 from 0
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
int findEnd(int p){
	int end = p;
	while(end < total && (!is_white_char(dyn_buf[end])) )
		end++;

	return end;	
}

/*
* Checks whether a file is a directory.
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

/*
* Copies from size bytes from buffer to cutoff starting at index in buffer
*/
void copy_to_dyn_buf(){
	int index = total;
	for(int i = 0; i < bytes_read; i++, index++)
		dyn_buf[index] = buf[i];
	 
}

void normalize(){
	dyn_buf = realloc(dyn_buf, total + bytes_read);
	if(dyn_buf == NULL){
		perror("Memory allocation error");
		puts("asdfsadf");
		return;
	}

	copy_to_dyn_buf();
	total += bytes_read;

	int first_char = 1;
	int space_count = 0;
	int newline_count = 0;
	int line_start = 0;
	int size = 0;

	for(int i = 0; i < total; i++){
		char curr = dyn_buf[i];

		if(first_char){
			if(is_white_char(curr)){
				int p = i;
				while(p < total && is_white_char(dyn_buf[p]))
					p++;

				if(p >= total){
					if(i == 0)
						total = 0;
					else
						total = i /*- 1*/;
							
					break;
				}
				memmove(dyn_buf + i, dyn_buf + p, total - p);
				total = total - (p - i);
			}
			line_start = i;
			curr = dyn_buf[i];
			first_char = 0;	
			size++;
			newline_count = 0;
			space_count = 0;
		}

		if(size == line_size){
			if(curr == '\n'){
				if(space_count == 1){
					memmove(dyn_buf + (i - 1), dyn_buf + i, total - i);
					total--;	
					space_count = 0;
					i--;

				}else if (newline_count == 1){
					newline_count = 0;
					first_char = 1;
				}

				size = 0;	
				first_char = 1;
				continue;
				
			}else{
				if(is_white_char(curr)){
					if(space_count == 1){
						memmove(dyn_buf + (i - 1), dyn_buf + i, total - i);
						total--;	
						space_count = 0;
						dyn_buf[--i] = '\n';		
					}else if(newline_count == 1){
						i--;
						newline_count = 0;
					}else{
						dyn_buf[i] = '\n';
					}
				
					size = 0;
					first_char = 1;
					continue;
				}else{
					int start = findStart(i);
					int end = findEnd(i);
					if(start >= line_start){
						dyn_buf[start] = '\n';
						i = start;
						size = 0;
						first_char = 1;
					}else if (end < total){
						dyn_buf[end] = '\n';
						i = end;
						size = 0;
						first_char = 1;	
						fail = 1;
					}else if (end >= total && start <= line_start){
						fail = 1;
					//	word_too_big = 1;	
						break;
					}
					continue;
				}
			}
		}else{ // not last char in line case
			if(is_white_char(curr)){
				if(curr == '\n'){
					if(newline_count == 0){
						newline_count = 1;
						size++;	
					}else if(newline_count == 1){
						newline_count = 0;
						size = 0;
						first_char = 1;

						//if(i == (bytes_read - 1))
						//	bytes_read--;
					}
					if(space_count == 1){
						memmove(dyn_buf + i - 1, dyn_buf + i, total - i);
						total--;	
						i--;
						space_count = 0;
						newline_count = 1;
					}

				}else if(curr == ' '){
					if(space_count == 1 || newline_count == 1){
						memmove(dyn_buf + i - 1, dyn_buf + i, total - i);
						total--;
						i--;
						newline_count = 0;
					}
						space_count = 1;
						size++;
				}else{
					if(space_count == 1){
						memmove(dyn_buf + i - 1, dyn_buf + i, total - i);
						total--;	
						i--;
						space_count = 1;
						dyn_buf[i] = ' ';
					}else if (newline_count == 1){
						memmove(dyn_buf + i - 1, dyn_buf + i, total - i);
						total--;	
						i--;
						newline_count = 1;
						dyn_buf[i] = '\n';
					}else{
						dyn_buf[i] = ' ';	
						space_count = 1;
					}
				}	
			}else{
				if(newline_count == 1){
					dyn_buf[i-1] = ' ';
				}
				size++;
				newline_count = 0;
				space_count = 0;	
			}
			 
		}
	}
}

/*
* The goal of this function is to read all bytes from file, normalize the buffer as bytes
* are read, then print the buffer after the chars have been normalized.
*/
void writeFile(int line_size, int ofd, int fd){
	//reads from file if bytes_read > 0 it continues to read from file until bytes_read < 1	
	do{	
		bytes_read = read(fd, buf, buf_size);
		if(bytes_read <= 0)
			break;
		
		normalize();
	}while(bytes_read > 0);

	if(total > 2 && dyn_buf[total - 1] == '\n' && dyn_buf[total - 2] == '\n') 
		total--;

	write(ofd, dyn_buf, total);
	if(total > 0 && dyn_buf[total - 1] != '\n')
		newLine(ofd);

	free(dyn_buf);
	dyn_buf = NULL;
	total = 0;
}

/*
* Takes a char * that contains the file name of the file we want to wrap
* returns a char * containing with wrap. added before name
*/
char *getFileName(char *name){
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

int main(int argc, char **argv){
	//Validate user input and initialize
	if(argc < 2){
		return EXIT_FAILURE;
	}else if(argc == 2 && atoi(argv[1]) > 0){
		
		buf = malloc(sizeof(char) * buf_size);
		fd = STDIN_FILENO;
		ofd = STDOUT_FILENO;		
		line_size = atoi(argv[1]);
		writeFile(line_size, ofd, fd);

	}else if (argc == 3){
		if(isDirectory(argv[2])){	
			buf = malloc(sizeof(char) * buf_size);

			char *dir_name = argv[2];
			DIR *directory = opendir(dir_name);
			if(directory == NULL){
				perror("Failed to open directory\n");
				return EXIT_FAILURE;
			}

			struct dirent *entry;
			line_size = atoi(argv[1]) + 1;
			entry = readdir(directory);
			chdir(dir_name);

			while(entry != NULL){
				char *name = entry->d_name;
				if(isRegularFile(name) && name[0] != '.' && strncmp("wrap.", name, strlen("wrap.")) != 0){
					char *output_filename = getFileName(name);	

					fd = open(name, FFLAGS);
					ofd = open(output_filename, WRITE_FLAGS, MODE);
					writeFile(line_size, ofd, fd);
					
					if(ofd < 0 || fd < 0){
						perror("Could not open file");
						EXIT_FAILURE;
					}

					free(output_filename);
					close(ofd);
					close(fd);
				}
				entry = readdir(directory);
			}

			closedir(directory);
		}else{
			fd = open(argv[2], FFLAGS);
			ofd = STDOUT_FILENO;
			line_size = atoi(argv[1]) + 1; 

			if(fd < 0 || line_size - 1 < 1){ 
				perror("Could not open file");
				return EXIT_FAILURE;
			}	

			buf = malloc(sizeof(char) * buf_size);
			writeFile(line_size, ofd, fd);
		}
	}else
		return EXIT_FAILURE;

	free(buf);

	if(fail)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;

}
