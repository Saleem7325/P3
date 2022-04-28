#include "word_wrap.h"

const char white_chars[6] = {'\t', '\n', ' ', '\v', '\f', '\r'};

/*---------------------------------------------------- Initialize word_wrap attributes -----------------------------------------------------------*/

/*
* Receives a word_wrap *ww, and an int size which indicates line size. init_word_wrap sets relevant fields in ww struct
* such that when write_file is passed ww struct pointer, write_file has necessary information for wrapping and writing.
*/
word_wrap *init_word_wrap(word_wrap *ww, int size){
	ww->dyn_buf = NULL;	
	ww->total = 0;
	ww->line_size = size; 
	ww->total = 0;
	ww->bytes_read = 0;
	ww->line_start = 0;
	ww->pe = 0;
	return ww;
}

/*------------------------------------------------------- Write File Helper functions ------------------------------------------------------------*/

/*
* Receives a word_wrap *ww, and an int code which specifies which error message to print. p_error gets file data
* from ww to print error messages with relevant file info. 
*/
void p_error(word_wrap *ww, int code){
	switch(code){
		case 1 : 
			perror("Unreadable input file");
			return;  
		case 2:
			perror("Unable to create output file");
			return;
		case 3:
			if(ww->ofd != 1){
				errno = EINVAL; 
				fprintf(stderr, "Word(s) longer than line size: %d in %s : %s\n", ww->line_size, ww->path, /*(ww->total/(*//*+ 1*/ strerror(errno));
			}else{
			
				errno = EINVAL; 
				fprintf(stderr, "Word(s) longer than line size: %d : %s\n", ww->line_size, strerror(errno));
			}
			ww->pe = 1;
			return;
		default:
			return;	
	}
}

/*
* Receives a char *name that contains the file name of the file we want to wrap returns a char * containing with "wrap." added before name
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

/*
* Receives a char *path, which correlates to ww->ifd. get_file_path returns the path relevant to ww->ofd or the output file
* created by write_file ex: directory/directory/filename -> directory/directory/wrap.filename  
*/
char *get_file_path(char *path){
	int len = strlen(path);
	int i = len - 1;
	while(i > -1 && path[i] != '/')
		i--;

	if(i > -1){
		char *file_name = malloc(len - i);
		file_name[len - i - 1] = '\0';
		strncpy(file_name, path + i + 1, len - i - 1); 			
		char *out_file_name = get_file_name(file_name);	
		char *out_file_path = malloc(strlen(out_file_name) + i + 2);
		out_file_path[strlen(out_file_name) + i + 1] = '\0';
		strncpy(out_file_path, path, i + 1); 			
		strncpy(out_file_path + i + 1, out_file_name, strlen(out_file_name)); 			
		free(file_name);
		free(out_file_name);	
		return out_file_path;
	}else
		return get_file_name(path);
			
}

/*
* Receives a word_wrap *ww, char *path, and int file_arg. set_file_decriptors opens the file pertaining to path and sets ww->ifd. If
* file_arg = 1, ww->ofd(output file descriptor) is set to STDOUT_FILENO, otherwise the relevant output file is opened and ww->ofd is set 
* accordingly. If at any point a file cannot be opened/does not exist an error message is printed and 1 it returned otherwise 0 is returned
*/
int set_file_descriptors(word_wrap *ww, char *path, int file_arg){
	if(strcmp(path, "STDIN_FILENO") == 0){
		ww->ifd = STDIN_FILENO;
		ww->ofd = STDOUT_FILENO; 
		return 0;
	}

	ww->ifd = open(path, READ_FLAGS);
	if(ww->ifd < 0){
		p_error(ww, 2);
		return 1;
	}else if(file_arg){
		ww->ofd = STDOUT_FILENO; 
		return 0;		
	}
		
	char *file_path = get_file_path(path);
	ww->ofd = open(file_path, WRITE_FLAGS, MODE);
	ww->path = file_path;

	if(ww->ofd < 0){
		p_error(ww, 2);
		free(file_path);
		return 1;
	}	
	
	return 0;
}

/*-------------------------------------------------------------- Normalize helper functions ---------------------------------------------------------*/

/*
* Recieves a word_wrap *ww, writes '\n' to ww->ofd.
*/
void newLine(word_wrap *ww){
	char p = '\n';
	write(ww->ofd, &p, 1);
}

/*
* Checks if character passed is in white_chars
*/
int is_white_char(char c){
	for(int i = 0; i < 6; i++)
		if(white_chars[i] == c)
			return 1;
	return 0;
}

/*
* Recieves a word_wrap *ww, and int p, given p findStart will find the first index that is a white space 
* starting at p and decrementing until a whitespace is reached. If p reaches 0 without encountering a 
* white space 0 is returned otherwise the index of the first previous white space is returned. 
*/
int findStart(word_wrap *ww, int p){
	int start = p;
	while(start > -1 && (!is_white_char(ww->dyn_buf[start]))) 
		start--;

	if(start < 0)
		return 0;
	return start;
}

/*
* Recieves a word_wrap *ww, and int p, given p findEnd will find the first index that is a white space 
* starting at p and incrementing until a whitespace is reached. If p reaches ww->total without encountering a 
* white space total is returned otherwise the index of the first following white space is returned.
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
* Recieves a word_wrap *ww, copies ww->bytes_read bytes from ww->buf[0] to ww->dyn_buf[ww->total : ww->total+bytes_read]
*/
void copy_to_dyn_buf(word_wrap * ww){
	int index = ww->total;
	for(int i = 0; i < ww->bytes_read; i++, index++)
		ww->dyn_buf[index] = ww->buf[i];
	 
}

/*
* Recieves word_wrap *ww, and int i which is the index to the first character in a line in ww->dyn_buf. first_char_white returns the 
* index of the next non white space character in ww->dyn_buf following ww->dyn_buf[*i] and moves bytes ww->dyn_buf[next_non_white_char : total] 
* to ww->dyn_buf[i] and adjusts ww->total to reflect these changes. If no non white space character is found i is returned and ww->total
* adjusted. 
*/
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
				ww->total = i;
						
			return 0;
		}
		memmove(ww->dyn_buf + i, ww->dyn_buf + p, ww->total - p);
		ww->total = ww->total - (p - i); 
	}
	return 1;
}

/*
* Recieves word_wrap *ww, and int *i which is an index to a character in ww->dyn_buf. shift_left moves bytes ww->dyn_buf[i : total]  
* to ww->dyn_buf[i-1] to overwrite a byte in ww->dyn_buf. ww->total and *i are adjusted to reflect these changes in ww->dyn_buf 
*/
void shift_left(word_wrap * ww, int *i){
	memmove(ww->dyn_buf + *i - 1, ww->dyn_buf + *i, ww->total - *i);
	ww->total--;	
	(*i)--;
}


/*
* Recieves word_wrap *ww, and various other pointers pertaining to loop iteration/flags in normalize. last_char_in_line determines 
* where the line at ww->dyn_buf[*i] should end. If the word needs to be excluded from the line in order to meet ww->line_size width
* a new line char is written to the first white space before ww->dyn_buf[*i], if the word cannot fit on the line the next white 
* space following ww->dyn_buf[*i] is replaced with a new line and an error message is printed indicating that a word is too 
* large to meet ww->line_size width. In both cases 0 is returned and normalize continues. If no white space character is found between 
* ww->dyn_buf[ww->line_start : *i] or ww->dyn_buf[*i : ww->total] an error message is printed and 1 is returned to signal normalize to
* break.
*/
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
			if(!(ww->pe))
				p_error(ww, 3); 
		}else if (end >= ww->total && start <= ww->line_start){
			if(!(ww->pe))
				p_error(ww, 3); 
			return 0;
		}
	}
	return 1;
}

/*
* Recieves word_wrap *ww, and various other pointers pertaining to loop iteration/flags in normalize. not_last_char_in_line behaves 
* according to the values of the flags passed by normalize. Whether to set flags and continue or overwrite a previous white space character
* is determined by the value of the flags(space_count, newline_count, first_char, size) along with the value of ww->dyn_buf[*i]. To simply
* explain not_last_char_in_line we look which case ww->dyn_buf[*i] fits into:
*
*	1) Non white space character: if previous character is '\n' replace with ' ' otherwise return.
*
*	2) White space character: 
*		- if '\n' or ' ', and space_count and newline_count = 0 set relevant flags
*		- if '\n', and space_count = 1 shift ww->dyn_buf[*i : ww->total] left one index 
*		- if ' ', and newline_count = 1 shift ww->dyn_buf[*i : ww->total] left one index
*		- if neither '\n' or ' ', and space_count or newline_count is 0 replace ww->dyn_buf[*i] with ' ' 
*		- if neither '\n' or ' ', and space_count or newline_count is 1  replace ww->dyn_buf[*i] with ' ' and
*		  shift ww->dyn_buf[*i : ww->total] left one index
*		  
*/
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
				(*size)--;
				*space_count = 0;
				*newline_count = 1;
			}

		}else if(curr == ' '){
			if(*space_count == 1 || *newline_count == 1){
				shift_left(ww, i);
				*newline_count = 0;
				*space_count = 1;
			}else{ 
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

/*
* Recieves a word_wrap *ww. Called by write_file after reading from ww->ifd into ww->buf and updating ww->bytes_read. Given ww, normalize
* copies ww->bytes_read bytes from ww->buf to ww->dyn_buf[ww->total : ww->total + ww->bytes_read] then normalizes the new bytes copied to 
* ww->dyn_buf. After normalize finishes executing ww->dyn_buf will contain only normalized text wrapped to ww->line_size width if no word
* is larger than ww->line_size in ww->dyn_buf. 
*/
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
		}else{ 
			not_last_char_in_line(ww, &i, &space_count, &newline_count, &first_char, &size);
		}
	}
}

/*--------------------------------------------------------------------- Write File ----------------------------------------------------------------------*/

/*
* Receives a word_wrap *ww, char *path, int file_arg. write_file opens necessary files and repeatedly reads from ww->ifd(input file decriptor
* correlating to path) and normalizes the bytes read from ww->ifd until a value less than or equal to 0 is returned from read. After reading/
* normalizing is finished ww->total bytes are written from ww->dyn_buf to ww->ofd(output file descriptor). If for any reason a file can't be
* opened/created write_file returns without reading/writing any bytes and prints relevant error message.
*/
void write_file(word_wrap *ww, char *path, int file_arg){
	if(set_file_descriptors(ww, path, file_arg))
		return;

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
	if(!file_arg)
		free(ww->path);
	ww->dyn_buf = NULL;
	ww->path = NULL;
	ww->pe = 0;
	ww->total = 0;
	ww->line_start = 0;
}

