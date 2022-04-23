TARGET = p3
CC = gcc
CFLAGS = -g -std=gnu99 -pthread -Wall -Wvla -Werror -fsanitize=address,undefined
DEPENDENCIES = DS/directory_stack.c DS/directory_stack.h  DS/file_stack.c DS/file_stack.h word_wrap/word_wrap.c word_wrap/word_wrap.h 

$(TARGET): $(TARGET).c $(DEPENDENCIES) 
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(TARGET) *.o *.a *.dylib *.dSYM

valgrind: $(TARGET).c
	$(CC) ww.c -o ww -g
	valgrind ./ww 20 tests
