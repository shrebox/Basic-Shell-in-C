compile: newterm newterm.c ush ush.c
	gcc -o newterm newterm.c
	gcc -o ush ush.c
	./newterm


