CREATE or replace FUNCTION pg_hist(query text, nbins int8[], 
    minimums float8[], maximums float8[]) RETURNS record
    AS '/home/koposov/pg_hist/pg_hist.so'
    LANGUAGE C;

CREATE or replace FUNCTION pg_hist_1d(query text, nbins int8[], 
     minimums float8[], maximums float8[],
     x1 out int , hist out bigint ) RETURNS setof record
    AS '/home/koposov/pg_hist/pg_hist.so'
    LANGUAGE C;
    
CREATE or replace FUNCTION pg_hist_2d(query text, nbins int8[], 
     minimums float8[], maximums float8[],
     x1 out int , x2 out int, hist out bigint ) RETURNS setof record
    AS '/home/koposov/pg_hist/pg_hist.so'
    LANGUAGE C;

CREATE or replace FUNCTION pg_hist_3d(query text, nbins int8[],
     minimums float8[], maximums float8[],
     x1 out int , x2 out int, x3 out int, hist out bigint ) RETURNS setof record
    AS '/home/koposov/pg_hist/pg_hist.so'
    LANGUAGE C;
    
CREATE or replace FUNCTION pg_hist_w(query text, nbins int8[], 
    minimums float8[], maximums float8[]) RETURNS record
    AS '/home/koposov/pg_hist/pg_hist.so'
    LANGUAGE C;

CREATE or replace FUNCTION pg_hist_1d_w(query text, nbins int8[], 
     minimums float8[], maximums float8[],
     x1 out int , hist out bigint ) RETURNS setof record
    AS '/home/koposov/pg_hist/pg_hist.so'
    LANGUAGE C;
    
CREATE or replace FUNCTION pg_hist_2d_w(query text, nbins int8[], 
     minimums float8[], maximums float8[],
     x1 out int , x2 out int, hist out bigint ) RETURNS setof record
    AS '/home/koposov/pg_hist/pg_hist.so'
    LANGUAGE C;

CREATE or replace FUNCTION pg_hist_3d_w(query text, nbins int8[],
     minimums float8[], maximums float8[],
     x1 out int , x2 out int, x3 out int, hist out bigint ) RETURNS setof record
    AS '/home/koposov/pg_hist/pg_hist.so'
    LANGUAGE C;