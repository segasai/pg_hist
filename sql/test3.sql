select * from pg_hist('select a from test1 where a is null',ARRAY[1], ARRAY[0],ARRAY[2]) 
    as (a int, b int);
select * from pg_hist_1d('select a from test1 where a is null',ARRAY[1], ARRAY[0],ARRAY[2]);
-- checks for out of hist cases
select * from pg_hist_1d('select -1000::real from test1',
            ARRAY[100],ARRAY[0],ARRAY[100]);
select * from pg_hist_1d('select -1000::double precision from test1',
            ARRAY[100],ARRAY[0],ARRAY[100]);
select * from pg_hist_1d('select -1000::int from test1',
            ARRAY[100],ARRAY[0],ARRAY[100]);
select * from pg_hist_1d('select -1000::bigint from test1',
            ARRAY[100],ARRAY[0],ARRAY[100]);
select * from pg_hist_2d('select -1000::bigint,-1000. from test1',
        ARRAY[100,100],ARRAY[0,0],ARRAY[100,100]);
select * from pg_hist_3d('select -1000::bigint,-1000.,-100::int from test1',
        ARRAY[100,100,100],ARRAY[0,0,0],ARRAY[100,100,100]);
select * from pg_hist_1d('select 1000 from test1',
            ARRAY[100],ARRAY[0],ARRAY[100]);
select * from pg_hist_2d('select 1000::bigint,1000. from test1',
        ARRAY[100,100],ARRAY[0,0],ARRAY[100,100]);
select * from pg_hist_3d('select 1000::bigint,1000.,100::int from test1',
        ARRAY[100,100,100],ARRAY[0,0,0],ARRAY[100,100,100]);
