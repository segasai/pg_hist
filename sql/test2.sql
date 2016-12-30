select * from pg_hist_w('select a,1 from test1',ARRAY[1], ARRAY[0],ARRAY[2]) 
    as (a int, b double precision);

select * from pg_hist_1d_w('select a,1 from test1',ARRAY[1], ARRAY[0],ARRAY[2]);
select * from pg_hist_1d_w('select b,1 from test1',ARRAY[1], ARRAY[0],ARRAY[2]);
select * from pg_hist_1d_w('select c,1 from test1',ARRAY[1], ARRAY[0],ARRAY[2]);
select * from pg_hist_1d_w('select d,1 from test1',ARRAY[1], ARRAY[0],ARRAY[2]);
select * from pg_hist_1d_w('select e,1 from test1',ARRAY[1], ARRAY[0],ARRAY[2]);

select * from pg_hist_1d_w('select a,1 from test1',ARRAY[100], ARRAY[-0.1],ARRAY[99.9]);
select * from pg_hist_1d_w('select b,1 from test1',ARRAY[100], ARRAY[-0.1],ARRAY[99.9]);
select * from pg_hist_1d_w('select c,1 from test1',ARRAY[100], ARRAY[-0.1],ARRAY[99.9]);
select * from pg_hist_1d_w('select d,1 from test1',ARRAY[100], ARRAY[-0.1],ARRAY[99.9]);
select * from pg_hist_1d_w('select e,1 from test1',ARRAY[100], ARRAY[-0.1],ARRAY[99.1]);

select * from pg_hist_2d_w('select a,b,1 from test1',ARRAY[100,100], ARRAY[-0.1,-0.1],ARRAY[99.9,99.9]);
select * from pg_hist_2d_w('select b,c,1 from test1',ARRAY[100,100], ARRAY[-0.1,-0.1],ARRAY[99.9,99.9]);
select * from pg_hist_2d_w('select c,d,1 from test1',ARRAY[100,100], ARRAY[-0.1,-0.1],ARRAY[99.9,99.9]);
select * from pg_hist_2d_w('select d,e,1 from test1',ARRAY[100,100], ARRAY[-0.1,-0.1],ARRAY[99.9,99.9]);
select * from pg_hist_2d_w('select e,a,1 from test1',ARRAY[100,100], ARRAY[-0.1,-0.1],ARRAY[99.9,99.9]);

select * from pg_hist_2d_w('select a,b*2,1 from test1',ARRAY[100,100], ARRAY[-0.1,-0.1],ARRAY[99.9,99.9]);
select * from pg_hist_2d_w('select b,c*2,1 from test1',ARRAY[100,100], ARRAY[-0.1,-0.1],ARRAY[99.9,99.9]);
select * from pg_hist_2d_w('select c,d*2,1 from test1',ARRAY[100,100], ARRAY[-0.1,-0.1],ARRAY[99.9,99.9]);
select * from pg_hist_2d_w('select d,e*2,1 from test1',ARRAY[100,100], ARRAY[-0.1,-0.1],ARRAY[99.9,99.9]);
select * from pg_hist_2d_w('select e,a*2,1 from test1',ARRAY[100,100], ARRAY[-0.1,-0.1],ARRAY[99.9,99.9]);

select * from pg_hist_1d_w('select 1.,1 from generate_series(1,100000);',ARRAY[1], ARRAY[0],ARRAY[2]);
select * from pg_hist_2d_w('select 1.,1.,1 from generate_series(1,100000);',ARRAY[1,1], ARRAY[0,0],ARRAY[2,2]);
select * from pg_hist_3d_w('select 1.,1.,1.,1 from generate_series(1,100000);',ARRAY[1,1,1], ARRAY[0,0,0],ARRAY[2,2,2]);

select * from pg_hist_w('select 1.,1.,1.,1.,1.,1 from generate_series(1,100000);',ARRAY[11,11,11,11,11], ARRAY[0,0,0,0,0],ARRAY[2,2,2,2,2]) 
    as (x1 int, x2 int, x3 int, x4 int, x5 int, hist double precision);
