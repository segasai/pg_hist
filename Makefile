EXTENSION = pg_hist
EXTVERSION := 1.0.0
DOCS = README.md
OBJS = pg_hist.o
MODULE_big = pg_hist
DATA = $(wildcard scripts/*sql)
PG_CONFIG = pg_config


PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)


#FLAGS=-fPIC -Wall -g -ggdb -O0
#all: 
#	gcc $(FLAGS) -I/home/koposov/pg_install/include/postgresql/server pg_hist.c -c
#	gcc -L/home/koposov/pg_install/lib pg_hist.o -shared -o pg_hist.so
	