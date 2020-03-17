all: server client

server: server.c
	gcc -o server server.c

client: client.c
	gcc -o client -pthread client.c

.PHONY: clean
clean:
	@rm -f server client
	@echo Cleaned.
