CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -Werror -Wall -g
LDFLAGS ?= -pthread -lrt

SRC := aesdsocket.c
TARGET ?= aesdsocket

all: $(TARGET)

$(TARGET) : $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) aesdsocket.c $(LDFLAGS)

clean:
	rm -f *.o $(TARGET)
