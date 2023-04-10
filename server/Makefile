CC=$(CROSS_COMPILE)gcc
CFLAGS=-Werror -Wall -g

all: aesdsocket

clean:
	rm -f aesdsocket.o aesdsocket

aesdsocket: aesdsocket.o
	$(CC) $(CFLAGS) -o aesdsocket aesdsocket.o	


