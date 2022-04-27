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

/*
* - buffer of size 1024 bytes for reading from ifd(input file descriptor) pertaining to path
* - dyn_buf is a dynamic buffer for storing normalized bytes
* - total is the number of bytes in dyn_buf
* - line_size is the number of characters that should be printed before each '\n' (one '\n' sequence).
* - ifd is the file descriptor to read from(input file descriptor)
* - ofd is the file descriptor to write to(output file descriptor)
* - bytes_read is the number of bytes read in previous call to read
* - line_start is the index of the first byte in the most recent line that has not ended
* - pe is a flag that indicates whether a word to big error has been printed before
* - path contains the path which corresponds to ifd
*/
typedef struct word_wrap{
	char buf[BUFSIZE];
	char *dyn_buf;
	int total;
	int line_size; 
	int ifd; 
	int ofd; 
	int bytes_read;
	int line_start;
	int pe;
	char *path;
}word_wrap;

word_wrap *init_word_wrap(word_wrap *ww, int size);

void write_file(word_wrap *ww, char *path, int file_arg);
