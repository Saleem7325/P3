TARGET = ww
CC = gcc
CFLAGS = -g -std=c99 -pthread -Wall -Wvla -Werror -fsanitize=address,undefined
DEPENDENCIES = DS/directory_stack.c DS/directory_stack.h  DS/file_stack.c DS/file_stack.h word_wrap/word_wrap.c word_wrap/word_wrap.h 

$(TARGET): $(TARGET).c $(DEPENDENCIES) 
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(TARGET) *.o *.a *.dylib *.dSYM

valgrind: $(TARGET).c
	$(CC) p3.c $(DEPENDENCIES) -o p3 -g
	valgrind ./p3 -r150,45 20 tests
