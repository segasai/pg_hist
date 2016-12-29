CREATE or replace FUNCTION pg_hist(text, int8[], float8[], float8[]) RETURNS integer
    AS '/home/koposov/pg_hist/pg_hist.so'
    LANGUAGE C;