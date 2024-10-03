CC=gcc
CFLAGS=-Wall -g -fdiagnostics-color=always -I./backend/include -I./backend/http -I./backend/core
TARGET=webproxy
SRC= \
	backend/main.c \
	backend/http/http_server.c \
	backend/http/http_fetch.c \
	backend/http/http_parse.c \
	backend/http/http_cookies.c \
	backend/http/http_session.c \
	backend/core/proxy.c \
	backend/core/session.c \
	backend/core/uniques.c \
	backend/core/hash.c 
LIBS= -lmicrohttpd -lcurl -luriparser -luuid
all: $(TARGET)

$(TARGET): 
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LIBS)

clean: 
	rm -rf $(TARGET)