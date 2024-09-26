CC=gcc
CFLAGS=-Wall -g -fdiagnostics-color=always -I./backend/include
TARGET=webproxy
SRC= \
	backend/main.c \
	backend/http/http_server.c \
	backend/http/http_fetch.c \
	backend/http/http_parse.c
LIBS= -lmicrohttpd -lcurl -luriparser
all: $(TARGET)

$(TARGET): 
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LIBS)

clean: 
	rm -rf $(TARGET)