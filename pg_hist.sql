CREATE or replace FUNCTION pg_hist(text, int8[], float8[], float8[]) RETURNS record
    AS '/home/koposov/pg_hist/pg_hist.so'
    LANGUAGE C;


CREATE or replace FUNCTION pg_hist_1d(text, int8[], float8[], float8[],
     out int , out bigint ) RETURNS setof record
    AS '/home/koposov/pg_hist/pg_hist.so'
    LANGUAGE C;
    
CREATE or replace FUNCTION pg_hist_2d(text, int8[], float8[], float8[],
     out int , out int, out bigint ) RETURNS setof record
    AS '/home/koposov/pg_hist/pg_hist.so'
    LANGUAGE C;

CREATE or replace FUNCTION pg_hist_3d(text, int8[], float8[], float8[],
     out int , out int, out int, out bigint ) RETURNS setof record
    AS '/home/koposov/pg_hist/pg_hist.so'
    LANGUAGE C;