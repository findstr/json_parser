all:json.c json.h main.c
	gcc -Wall -g -o json $^
	./json
clean:
	rm json
