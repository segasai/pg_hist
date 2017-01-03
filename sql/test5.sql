select * from pg_hist_1d('select a from test1',ARRAY[1], ARRAY[0],ARRAY[2,2]);
select * from pg_hist_1d('select a from test1',ARRAY[-1], ARRAY[0],ARRAY[2]);
select * from pg_hist_2d('select a,b from test1',ARRAY[100,100,100], ARRAY[-0.1,-0.1],ARRAY[99.9,99.9]);
select * from pg_hist_3d('select a,b from test1',ARRAY[100,100], ARRAY[-0.1,-0.1],ARRAY[99.9,99.9]);
