
all: client server

client: client.c
	gcc -Wall -pedantic -O2 -pthread -DNDEBUG -D_GNU_SOURCE -std=gnu99 $^ -lm -o $@

server: server.c
	gcc -Wall -pedantic -O2 -pthread -DNDEBUG -D_GNU_SOURCE -std=gnu99 $^ -o $@

.PHONY : clean
clean : 
	-rm client server
