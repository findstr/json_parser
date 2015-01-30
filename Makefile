all:json.c json.h main.c
	gcc -Wall -g3 -o json $^
	./json
clean:
	rm json
