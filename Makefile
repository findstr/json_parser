all:json.c json.h main.c
	gcc -D NDEBUG -Wall -g3 -o json $^
	./json
clean:
	rm json
