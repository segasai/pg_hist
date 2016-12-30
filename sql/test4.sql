select * from pg_hist_2d('select b,(c::numeric)%100 from test2',ARRAY[30,30],ARRAY[0,0],ARRAY[100,100]);
