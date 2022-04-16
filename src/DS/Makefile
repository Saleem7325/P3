TARGET = test
CC = gcc
CFLAGS = -g -std=gnu99 -pthread -Wall -Wvla -Werror -fsanitize=address,undefined
DEPENDENCIES = queue.c queue.h

$(TARGET): $(TARGET).c $(DEPENDENCIES) 
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(TARGET) *.o *.a *.dylib *.dSYM

valgrind: $(TARGET).c
	$(CC) ww.c -o ww -g
	valgrind ./ww 20 tests
