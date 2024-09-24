CC=gcc
CFLAGS=-Wall -g
TARGET=webproxy
SRC=backend/main.c backend/http/http_server.c
LIBS= -lmicrohttpd #-lanotherlib
all: $(TARGET)

$(TARGET): 
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LIBS)

clean: 
	rm -rf $(TARGET)