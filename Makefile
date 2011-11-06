all:
	gcc -ofugal -g -Wall -lncurses -lasound -ltpl fugal.c

install:
	mkdir -p /usr/local/bin/
	cp fugal /usr/local/bin/
