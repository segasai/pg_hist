select * from pg_hist('select a from test1 where a is null',ARRAY[1], ARRAY[0],ARRAY[2]) 
    as (a int, b int);
select * from pg_hist_1d('select a from test1 where a is null',ARRAY[1], ARRAY[0],ARRAY[2]);
