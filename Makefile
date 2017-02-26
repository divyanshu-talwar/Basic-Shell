ush: ush.c
	gcc -g -o ush ush.c -lreadline
	gnome-terminal -e ./ush

clean:
	rm -f ush