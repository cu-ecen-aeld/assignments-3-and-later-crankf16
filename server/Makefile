CC=$(CROSS_COMPILE)gcc
CFLAGS= -Werror -Wall -g
LDFLAGS?= -lpthread -lrt

all: aesdsocket

clean:
	rm -f aesdsocket.o aesdsocket

aesdsocket: aesdsocket.o
	$(CC) $(CFLAGS) -o aesdsocket aesdsocket.o $(LDFLAGS)	


