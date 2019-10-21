CC = gcc
CFLAGS = -g3 #-Wall -Wextra 

build: server

server: server.c
	$(CC) $(CFLAGS) -o $@ server.c

dist: 704966744.tar.gz
sources = Makefile server.c report.pdf README
704966744.tar.gz: $(sources)
	tar -cvzf $@ $(sources)

clean:
	rm -f 704966744.tar.gz server