create table test1 (a smallint, b int, c bigint, d real, e double precision);
insert into test1 (a,b,c,d,e) values (1,1,1,1,1);
insert into test1 (a,b,c,d,e) values (11,11,11,11,11);
insert into test1 (a,b,c,d,e) values (null, null, null, null, null);

create table test2 (b int, c double precision);
insert into test2 select generate_series%100,(generate_series/33.) from generate_series(0,100000);