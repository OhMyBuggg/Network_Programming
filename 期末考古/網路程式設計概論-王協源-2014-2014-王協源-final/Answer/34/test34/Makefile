all: server client

server: server.c ../writeline.h ../writeline_r.h
	gcc -o server -pthread server.c

client: client.c ../writeline.h ../writeline_r.h
	gcc -o client -pthread client.c

.PHONY: clean
clean:
	@rm -f server client
	@echo Cleaned.
