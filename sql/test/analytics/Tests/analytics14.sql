start transaction;
create table analytics (aa int, bb int, cc bigint);
insert into analytics values (15, 3, 15), (3, 1, 3), (2, 1, 2), (5, 3, 5), (NULL, 2, NULL), (3, 2, 3), (4, 1, 4), (6, 3, 6), (8, 2, 8), (NULL, 4, NULL);

select stddev_samp(aa) over (partition by bb) from analytics;
select stddev_samp(aa) over (partition by bb order by bb asc) from analytics;
select stddev_samp(aa) over (partition by bb order by bb desc) from analytics;
select stddev_samp(aa) over (order by bb desc) from analytics;

select stddev_samp(cc) over (partition by bb) from analytics;
select stddev_samp(cc) over (partition by bb order by bb asc) from analytics;
select stddev_samp(cc) over (partition by bb order by bb desc) from analytics;
select stddev_samp(cc) over (order by bb desc) from analytics;


select stddev_pop(aa) over (partition by bb) from analytics;
select stddev_pop(aa) over (partition by bb order by bb asc) from analytics;
select stddev_pop(aa) over (partition by bb order by bb desc) from analytics;
select stddev_pop(aa) over (order by bb desc) from analytics;

select stddev_pop(cc) over (partition by bb) from analytics;
select stddev_pop(cc) over (partition by bb order by bb asc) from analytics;
select stddev_pop(cc) over (partition by bb order by bb desc) from analytics;
select stddev_pop(cc) over (order by bb desc) from analytics;


select var_samp(aa) over (partition by bb) from analytics;
select var_samp(aa) over (partition by bb order by bb asc) from analytics;
select var_samp(aa) over (partition by bb order by bb desc) from analytics;
select var_samp(aa) over (order by bb desc) from analytics;

select var_samp(cc) over (partition by bb) from analytics;
select var_samp(cc) over (partition by bb order by bb asc) from analytics;
select var_samp(cc) over (partition by bb order by bb desc) from analytics;
select var_samp(cc) over (order by bb desc) from analytics;


select var_pop(aa) over (partition by bb) from analytics;
select var_pop(aa) over (partition by bb order by bb asc) from analytics;
select var_pop(aa) over (partition by bb order by bb desc) from analytics;
select var_pop(aa) over (order by bb desc) from analytics;

select var_pop(cc) over (partition by bb) from analytics;
select var_pop(cc) over (partition by bb order by bb asc) from analytics;
select var_pop(cc) over (partition by bb order by bb desc) from analytics;
select var_pop(cc) over (order by bb desc) from analytics;


select median(aa) over (partition by bb) from analytics;
select median(aa) over (partition by bb order by bb asc) from analytics;
select median(aa) over (partition by bb order by bb desc) from analytics;
select median(aa) over (order by bb desc) from analytics;

select median(cc) over (partition by bb) from analytics;
select median(cc) over (partition by bb order by bb asc) from analytics;
select median(cc) over (partition by bb order by bb desc) from analytics;
select median(cc) over (order by bb desc) from analytics;


select median_avg(aa) over (partition by bb) from analytics;
select median_avg(aa) over (partition by bb order by bb asc) from analytics;
select median_avg(aa) over (partition by bb order by bb desc) from analytics;
select median_avg(aa) over (order by bb desc) from analytics;

select median_avg(cc) over (partition by bb) from analytics;
select median_avg(cc) over (partition by bb order by bb asc) from analytics;
select median_avg(cc) over (partition by bb order by bb desc) from analytics;
select median_avg(cc) over (order by bb desc) from analytics;


select quantile(aa, 0.2) over (partition by bb) from analytics;
select quantile(aa, 0.2) over (partition by bb order by bb asc) from analytics;
select quantile(aa, 0.2) over (partition by bb order by bb desc) from analytics;
select quantile(aa, 0.2) over (order by bb desc) from analytics;

select quantile(cc, 0.7) over (partition by bb) from analytics;
select quantile(cc, 0.7) over (partition by bb order by bb asc) from analytics;
select quantile(cc, 0.7) over (partition by bb order by bb desc) from analytics;
select quantile(cc, 0.7) over (order by bb desc) from analytics;


select quantile_avg(aa, 0.2) over (partition by bb) from analytics;
select quantile_avg(aa, 0.2) over (partition by bb order by bb asc) from analytics;
select quantile_avg(aa, 0.2) over (partition by bb order by bb desc) from analytics;
select quantile_avg(aa, 0.2) over (order by bb desc) from analytics;

select quantile_avg(cc, 0.7) over (partition by bb) from analytics;
select quantile_avg(cc, 0.7) over (partition by bb order by bb asc) from analytics;
select quantile_avg(cc, 0.7) over (partition by bb order by bb desc) from analytics;
select quantile_avg(cc, 0.7) over (order by bb desc) from analytics;


select stddev_samp(aa) over () from analytics;
select stddev_pop(aa) over () from analytics;
select var_samp(aa) over () from analytics;
select var_pop(aa) over () from analytics;
select median(aa) over () from analytics;
select median_avg(aa) over () from analytics;
select quantile(aa, 0.8) over () from analytics;
select quantile_avg(aa, 0.8) over () from analytics;


select stddev_samp(NULL) over () from analytics;
select stddev_pop(NULL) over () from analytics;
select var_samp(NULL) over () from analytics;
select var_pop(NULL) over () from analytics;
select median(NULL) over () from analytics;
select median_avg(NULL) over () from analytics;
select quantile(NULL, 0.8) over () from analytics;
select quantile(aa, NULL) over () from analytics;
select quantile(NULL, NULL) over () from analytics;
select quantile_avg(NULL, 0.8) over () from analytics;
select quantile_avg(aa, NULL) over () from analytics;
select quantile_avg(NULL, NULL) over () from analytics;

rollback;

select median_avg(aa) over (), median_avg(bb) over () from (values(NULL, 1),(1, 2)) as analytics(aa, bb); --even number of values for median_avg

create table stressme (aa varchar(64), bb int);
insert into stressme values ('one', 1), ('another', 1), ('stress', 1), (NULL, 2), ('ok', 2), ('check', 3), ('me', 3), ('please', 3), (NULL, 4);

select stddev_samp(aa) over (partition by bb) from stressme; --error, stddev_samp not available for string type
select stddev_samp() over (partition by bb) from stressme; --error, stddev_samp without parameters not available

drop table stressme;
