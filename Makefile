FLAGS=-fPIC -Wall -g -ggdb -O0
all: 
	gcc $(FLAGS) -I/home/koposov/pg_install/include/postgresql/server pg_hist.c -c
	gcc -L/home/koposov/pg_install/lib pg_hist.o -shared -o pg_hist.so
	