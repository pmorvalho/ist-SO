make: par-shell
	
par-shell: projeto.o commandlinereader.o
	gcc -o par-shell projeto.o commandlinereader.o

projeto.o: projeto.c commandlinereader.o
	gcc -Wall -g -c projeto.c

commandlinereader.o: commandlinereader.c commandlinereader.h
	gcc -Wall -g -c commandlinereader.c

clean:
	rm -rf *.o
