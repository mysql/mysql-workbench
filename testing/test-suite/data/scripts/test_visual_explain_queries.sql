-- test queries for visual explain
-- queries in each group depend on tables created inline right above them

--
--
drop table if exists t0, t1, t2, t3;
create table t1 (a int, b int, index a (a,b));
create table t2 (a int, index a (a));
create table t3 (a int, b int, index a (a));
insert into t1 values (1,10), (2,20), (3,30), (4,40);
create table t0(a int);
insert into t0 values (0),(1),(2),(3),(4),(5),(6),(7),(8),(9);
insert into t1 select rand()*100000+200,rand()*100000 from t0 A, t0 B, t0 C, t0 D;
insert into t2 values (2), (3), (4), (5);
insert into t3 values (10,3), (20,4), (30,5);
--
select * from t1,t2 where t2.a in (select a from t1 where t1.b <> 30)
and t1.b=t2.a group by 1;

--
set @opt_save=@@optimizer_switch;
set optimizer_switch='materialization=off';
set optimizer_switch='loosescan=off';
set optimizer_switch='index_condition_pushdown=off';
set optimizer_switch='mrr=off';
set optimizer_switch='firstmatch=off';

select * from t1,t2 where t2.a in (select a from t1 where t1.b <> 30)
and t1.b=t2.a group by 1;

--
--
drop table if exists t1,t2,t3;
CREATE TABLE t1 (i INT, key(i));
CREATE TABLE t2 (i INT);
INSERT  INTO t1 VALUES(1),(2),(3);
INSERT  INTO t2 VALUES(1),(2),(3);
--
SELECT * FROM t1 JOIN t2 USING(i);

--
--
drop table if exists t1,t2;
CREATE TABLE t1 (a INT NOT NULL, b CHAR(3) NOT NULL, PRIMARY KEY (a));
INSERT INTO t1 VALUES (1,'ABC'), (2,'EFG'), (3,'HIJ');
CREATE TABLE t2 (a INT NOT NULL,b CHAR(3) NOT NULL,PRIMARY KEY (a, b));
INSERT INTO t2 VALUES (1,'a'),(1,'b'),(3,'F');
--
SELECT t1.a, GROUP_CONCAT(t2.b) AS b FROM t1 LEFT JOIN t2 ON t1.a=t2.a GROUP BY
t1.a ORDER BY t1.b;

--
--
SELECT a, b FROM
(SELECT 1 AS a, 2 AS b
UNION ALL
SELECT 1 AS a, 2 AS b) t1
GROUP BY a
ORDER BY b DESC;

--
--
drop table t1,t2,t3;
CREATE TABLE t1 (pk INTEGER PRIMARY KEY, vc VARCHAR(20));
INSERT INTO t1 VALUES(7, 'seven'), (13, 'thirteen');
CREATE TABLE t2 (pk INTEGER PRIMARY KEY, vc1 VARCHAR(20), vc2 VARCHAR(20));
INSERT INTO t2 VALUES(7, 'seven', 's'), (14, 'fourteen', 'f');
CREATE TABLE t3 (pk INTEGER PRIMARY KEY, vc VARCHAR(20));
INSERT INTO t3 VALUES(5, 'f'), (6, 's'), (7, 's');
--
SELECT t2.vc1
FROM t2 JOIN t3 ON t2.vc2=t3.vc;

SELECT derived.vc1
FROM (SELECT t2.vc1
FROM t2 JOIN t3 ON t2.vc2=t3.vc) as derived;

SELECT derived.vc
FROM t1 AS derived
WHERE derived.vc IN (
SELECT t2.vc1
FROM t2 JOIN t3 ON t2.vc2=t3.vc);

SELECT derived.vc
FROM (SELECT * FROM t1) AS derived
WHERE derived.vc IN (
SELECT t2.vc1
FROM t2 JOIN t3 ON t2.vc2=t3.vc);


SELECT 
    a, b
FROM
    (SELECT 1 AS a, 2 AS b UNION ALL SELECT 1 AS a, 2 AS b union all select 3 as a, 4 as b order by 1) t1
GROUP BY a
ORDER BY b DESC;


--
-- from sakila
--
use sakila;


select 
	`a`.`actor_id` AS `actor_id`,
	`a`.`first_name` AS `first_name`,
	`a`.`last_name` AS `last_name`,
	group_concat(distinct concat(`c`.`name`,
				': ',
				(select 
						group_concat(`f`.`title`
								order by `f`.`title` ASC
								separator ', ')
					from
						((`film` `f`
						join `film_category` `fc` ON ((`f`.`film_id` = `fc`.`film_id`)))
						join `film_actor` `fa` ON ((`f`.`film_id` = `fa`.`film_id`)))
					where
						((`fc`.`category_id` = `c`.`category_id`)
							and (`fa`.`actor_id` = `a`.`actor_id`))))
		order by `c`.`name` ASC
		separator '; ') AS `film_info`
from
	(((`actor` `a`
	left join `film_actor` `fa` ON ((`a`.`actor_id` = `fa`.`actor_id`)))
	left join `film_category` `fc` ON ((`fa`.`film_id` = `fc`.`film_id`)))
	left join `category` `c` ON ((`fc`.`category_id` = `c`.`category_id`)))
group by `a`.`actor_id` , `a`.`first_name` , `a`.`last_name`;


--
-- from world;
-- 
use world;

-- attached subqueries

SELECT 
    a.Name, a.District
FROM
    City AS a
        INNER JOIN
    Country ON a.CountryCode = Country.Code
WHERE
    Country.Code = 'JPN'
        AND a.Population >= (SELECT 
            MAX(City.Population)
        FROM
            City
                INNER JOIN
            Country ON City.CountryCode = Country.Code
        WHERE
            Country.Name = 'Malaysia')
        and a.Population <= (SELECT 
            MIN(City.Population)
        FROM
            City
                INNER JOIN
            Country ON City.CountryCode = Country.Code
        WHERE
            Country.Name = 'Malaysia');


select count(*), (select count(*) from City where length(Name) < 3), (select count(*) from Country where length(Name) < 3) from Country;



-- sakila
select *, (select first_name from sakila.actor_info) as fn from sakila.nicer_but_slower_film_list

-- funny attached_condition
select
    c_count,
    count(*) as custdist
from
    (
        select
            c_custkey,
            count(o_orderkey) as c_count
        from
            customer left outer join orders on
                c_custkey = o_custkey
                and o_comment not like '%express%requests%'
        group by
            c_custkey
    ) as c_orders 
group by
    c_count
order by
    custdist desc,
    c_count desc;

