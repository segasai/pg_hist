# pg_hist
pg_hist  -- N-d histogramming functions for Postgresql

Developer: Sergey Koposov

* 1D histogram

If you have a table tab with column x then in order to create a 1D histogram with bine edges [-1,55] and 22 bins you need to query:

```
select * from pg_hist_1d('select x from tab', ARRAY[22], ARRAY[-1], ARRAY[55]);
 x1 | hist 
----+------
  0 | 3441
  1 | 3379
```
This query returns the id of the bin (starting from zero and number of objects in a given bin. The bins with zero objects arent' returned

* 2D histogram

The syntax for 2D histogram is similar. Three arrays need to be given  the number of bins, the minimum edges of the bins, the maximum value edges of the bins.

```
select * from pg_hist_2d('select x,y from tab', ARRAY[22,33], ARRAY[-1,-5], ARRAY[55,5.4]);
 x1 |  x2 | hist 
----+-----+----
  0 |  1  | 3441
  1 |  3  | 3379
```
In the case of 2-D histograms the coordinates of non-empty bins and the number counts of objects in those bins are returned.

* N-D histogram

A similar syntax exists for a histogram with arbitrary number of dimensions
```
select * from pg_hist('select x,y,z,a from tab', ARRAY[5,6,7,8], ARRAY[-1.,-1.1,-1.2,-1.3], ARRAY[9.1,9.2,9.3,9.4]) as 
                  (x1 int, x2 int, x3 int, x4 int, hist bigint);
```

* Histograms with weights

It is also possible to make histogram with weighting. In this case your query need to return one more column. The last column will be used as a weight. 
```
select * from pg_hist_1d_w('select x,w from tab', ARRAY[22], ARRAY[-1], ARRAY[55]);
 x1 | hist 
----+------
  0 | 10.4
  1 | 20.2
```
