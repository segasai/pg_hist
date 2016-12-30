select * from pg_hist('select a from test1',ARRAY[1], ARRAY[0],ARRAY[2]) 
    as (a int, b int);
select * from pg_hist_1d('select a from test1',ARRAY[1], ARRAY[0],ARRAY[2]);
select * from pg_hist_1d('select b from test1',ARRAY[1], ARRAY[0],ARRAY[2]);
select * from pg_hist_1d('select c from test1',ARRAY[1], ARRAY[0],ARRAY[2]);
select * from pg_hist_1d('select d from test1',ARRAY[1], ARRAY[0],ARRAY[2]);
select * from pg_hist_1d('select e from test1',ARRAY[1], ARRAY[0],ARRAY[2]);

select * from pg_hist_1d('select a from test1',ARRAY[100], ARRAY[-0.1],ARRAY[99.9]);
select * from pg_hist_1d('select b from test1',ARRAY[100], ARRAY[-0.1],ARRAY[99.9]);
select * from pg_hist_1d('select c from test1',ARRAY[100], ARRAY[-0.1],ARRAY[99.9]);
select * from pg_hist_1d('select d from test1',ARRAY[100], ARRAY[-0.1],ARRAY[99.9]);
select * from pg_hist_1d('select e from test1',ARRAY[100], ARRAY[-0.1],ARRAY[99.1]);

select * from pg_hist_2d('select a,b from test1',ARRAY[100,100], ARRAY[-0.1,-0.1],ARRAY[99.9,99.9]);
select * from pg_hist_2d('select b,c from test1',ARRAY[100,100], ARRAY[-0.1,-0.1],ARRAY[99.9,99.9]);
select * from pg_hist_2d('select c,d from test1',ARRAY[100,100], ARRAY[-0.1,-0.1],ARRAY[99.9,99.9]);
select * from pg_hist_2d('select d,e from test1',ARRAY[100,100], ARRAY[-0.1,-0.1],ARRAY[99.9,99.9]);
select * from pg_hist_2d('select e,a from test1',ARRAY[100,100], ARRAY[-0.1,-0.1],ARRAY[99.9,99.9]);

select * from pg_hist_2d('select a,b*2 from test1',ARRAY[100,100], ARRAY[-0.1,-0.1],ARRAY[99.9,99.9]);
select * from pg_hist_2d('select b,c*2 from test1',ARRAY[100,100], ARRAY[-0.1,-0.1],ARRAY[99.9,99.9]);
select * from pg_hist_2d('select c,d*2 from test1',ARRAY[100,100], ARRAY[-0.1,-0.1],ARRAY[99.9,99.9]);
select * from pg_hist_2d('select d,e*2 from test1',ARRAY[100,100], ARRAY[-0.1,-0.1],ARRAY[99.9,99.9]);
select * from pg_hist_2d('select e,a*2 from test1',ARRAY[100,100], ARRAY[-0.1,-0.1],ARRAY[99.9,99.9]);

select * from pg_hist_1d('select 1. from generate_series(1,100000);',ARRAY[1], ARRAY[0],ARRAY[2]);
select * from pg_hist_2d('select 1.,1. from generate_series(1,100000);',ARRAY[1,1], ARRAY[0,0],ARRAY[2,2]);
select * from pg_hist_3d('select 1.,1.,1. from generate_series(1,100000);',ARRAY[1,1,1], ARRAY[0,0,0],ARRAY[2,2,2]);

select * from pg_hist('select 1.,1.,1.,1.,1. from generate_series(1,100000);',ARRAY[11,11,11,11,11], ARRAY[0,0,0,0,0],ARRAY[2,2,2,2,2]) 
    as (x1 int, x2 int, x3 int, x4 int, x5 int, hist int);
