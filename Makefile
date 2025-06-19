
clean: 
	rm -f logger.txt
	rm -f run

run: space_invaders.c
	make clean
	gcc -o run space_invaders.c
