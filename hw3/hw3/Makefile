target: server.c client.c
	gcc server.c -o server
	gcc client.c -o client
	if [ -d "tom" ]; then rm -rf tom; fi
	if [ -d "frank" ]; then rm -rf frank; fi
clean:
	rm client
	rm server
