all: client server

client: client.c
	gcc -Wall -pthread -pedantic -O2 -DNDEBUG -D_GNU_SOURCE -std=gnu99 $^ -lm -o $@

server: server.c
	gcc -Wall -pthread -pedantic -O2 -DNDEBUG -D_GNU_SOURCE -std=gnu99 $^ -o $@