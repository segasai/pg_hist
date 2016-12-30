EXTENSION = pg_hist
EXTVERSION := 1.0.0
DOCS = README.md
OBJS = pg_hist.o
MODULE_big = pg_hist
DATA = $(wildcard scripts/*sql)
PG_CONFIG = pg_config

#PG_CPPFLAGS := -g -ggdb -O0

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
TESTDB=pghist_test
test:
	createdb $(TESTDB)
	psql -c 'create extension pg_hist' $(TESTDB)
	psql -c '\i sql/create_data.sql' $(TESTDB)	
	mkdir results
	psql -c '\i sql/test1.sql' $(TESTDB) > results/test1.out
	diff results/test1.out expected/test1.expected
	dropdb $(TESTDB)	