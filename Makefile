server:wrap.o server.o
	gcc wrap.o server.o -o server -L /usr/lib/mysql/ -lmysqlclient

wrap.o:wrap.c
	gcc -c wrap.c

server.o:server.c
	gcc -c server.c