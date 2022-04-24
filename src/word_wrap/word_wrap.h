#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>

#define MODE S_IRUSR|S_IWUSR
#define READ_FLAGS O_RDONLY 
#define WRITE_FLAGS O_CREAT|O_WRONLY|O_APPEND|O_TRUNC
#define BUFSIZE 1024 

//char const white_chars[6] = {'\t', '\n', ' ', '\v', '\f', '\r'};

typedef struct word_wrap{
	pthread_mutex_t lock;
	char buf[1024];
	char *dyn_buf;
	int total;
	int line_size; 
	int ifd; 
	int ofd; 
	int bytes_read;
	//int exit_code;
	int line_start;
	int pe;
	//int line;
	char *path;
}word_wrap;

word_wrap *init_word_wrap(word_wrap *ww, int size);

void write_file(word_wrap *ww, char *path, int file_arg);
