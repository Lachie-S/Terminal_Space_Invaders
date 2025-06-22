
clean: 
	rm -f logger.txt
	rm -f run

run: space_invaders.c
	make clean
	gcc -c invader_list.c -o temp1
	gcc -c space_invaders.c -o temp2
	gcc -o run temp1 temp2
	rm temp*
