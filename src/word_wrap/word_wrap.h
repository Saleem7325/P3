//#include <stdlib.h>
//#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>

typedef struct word_wrap{
	pthread_mutex_t lock;
	char buf[1024];
	char *dyn_buf;
	int total;
	int line_size; 
	int ifd; 
	int ofd; 
	int bytes_read;
	int exit_code;
	int line_start;
}word_wrap;

word_wrap *init_word_wrap(word_wrap *ww, int size);

void writeFile(word_wrap *ww, char *path);
