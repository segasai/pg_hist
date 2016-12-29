CREATE or replace FUNCTION pg_hist(text, integer) RETURNS integer
    AS '/home/koposov/pg_hist/pg_hist.so'
    LANGUAGE C;