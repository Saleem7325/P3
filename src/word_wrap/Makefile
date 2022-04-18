TARGET = word_wrap
CC = gcc
CFLAGS = -g -std=c99 -pthread -Wall -Wvla -Werror -fsanitize=address,undefined

$(TARGET): $(TARGET).c $(TARGET).h
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(TARGET) *.o *.a *.dylib *.dSYM
