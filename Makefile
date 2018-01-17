EXTENSION = pg_hist
EXTVERSION := 1.0.0
DOCS = README.md
OBJS = pg_hist.o
MODULE_big = pg_hist
DATA = $(wildcard scripts/*sql)
PG_CONFIG = pg_config

#PG_CPPFLAGS := -g -ggdb -O0
SHLIB_LINK += $(filter -lm, $(LIBS))

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
TESTDB=pghist_test
test:
	createdb $(TESTDB)
	psql -c 'create extension pg_hist' $(TESTDB)
	psql -c '\i sql/create_data.sql' $(TESTDB)	
	mkdir -p results
	psql -c '\i sql/test1.sql' $(TESTDB) > results/test1.out
	psql -c '\i sql/test2.sql' $(TESTDB) > results/test2.out
	psql -c '\i sql/test3.sql' $(TESTDB) > results/test3.out
	psql -c '\i sql/test4.sql' $(TESTDB) > results/test4.out
	psql -c '\i sql/test5.sql' $(TESTDB) > results/test5.out 2>&1
	diff results/test1.out expected/test1.expected
	diff results/test2.out expected/test2.expected
	diff results/test3.out expected/test3.expected
	diff results/test4.out expected/test4.expected
	dropdb $(TESTDB)	