# -redbase-windows-VS-
经过不断的点滴，基于redbase简单实现了从底层开发到最后实现增删改查并解析语法词法。

#数据库 sql语句：
1.进入到数据库创建界面输入数据库名

db_test

2.进入到数据库操作界面

/* Test semantic checking.  */


create table soaps(soapid  i, sname  c28, network  c4, rating  f);



create table stars(starid  i, stname  c20, plays  c12, soapid  i);



/* 加载数据 */

load soaps("../data/soaps.data");



/* print out contents of soaps */

print soaps;



/* build some indices on stars */

create index stars(starid);

create index stars(stname);



/*加载stars数据*/

load stars("../data/stars.data");



/*查看加载后的数据*/

select * from stars;

/*联合查询*/

select * from stars, soaps;

/* 联合查询其中部分属性*/

select soaps.network, rating from stars, soaps;

/*二元条件查询*/

select soaps.network, rating from stars, soaps where soaps.soapid = stars.soapid;

/*一元条件查询*/

select * from stars where stars.starid = 3;

/*order by子句*/

select * from soaps;

select * from soaps order by rating;

select * from soaps order by rating desc;

/*聚集函数*/

select min(rating), max(rating), count(*) from soaps;





/*group by order by 子句*/

select * from soaps，stars where stars. soapid = soaps. soapid order by rating group by network;





select * from stars;

delete from stars where stars.starid = 3;

select * from soap

select * from stars;

insert into stars (30,sasa,dd,8);

select * from stars;

/*聚集函数+group by子句的使用*/

select network,min(rating) from soaps group by network;





exit;
