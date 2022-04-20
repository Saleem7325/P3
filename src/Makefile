TARGET = p3
CC = gcc
CFLAGS = -g -std=gnu99 -pthread -Wall -Wvla -Werror -fsanitize=address,undefined
DEPENDENCIES = DS/queue.c DS/queue.h word_wrap/word_wrap.c word_wrap/word_wrap.h 

$(TARGET): $(TARGET).c $(DEPENDENCIES) 
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(TARGET) *.o *.a *.dylib *.dSYM

valgrind: $(TARGET).c
	$(CC) ww.c -o ww -g
	valgrind ./ww 20 tests
