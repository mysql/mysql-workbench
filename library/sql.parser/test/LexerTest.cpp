/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#include <windows.h>
#include <sstream>

#include "lexertest.h"
#include "myx_sql_tree_item.h"
#include "myx_sql_parser_public_interface.h"
#include "myx_lex_helpers.h"

void LexerTest::cppunit_assert(bool cond)
{
  if(!cond) {
    throw new LexerTestException();
  }
}

void LexerTest::testLexer()
{
  //basicLexerTest();
  //sqlTest();
  //basicParserTest();
  //stmtscanTest();
  //fileParse("c:/co/qqq.sql");
}

void LexerTest::basicLexerTest()
{
  //yyin = stdin;
  
  //lex_input_stream = new std::ifstream("c:/co/test 20051018 2143.sql");
  //lex_input_stream->rdbuf()->pubsetbuf(new char[10*1024*1024], 10*1024*1024);

  //lex_input_stream = new std::istringstream(
  //  "'some string' ident ident1 1ident 111 x'123456' 111.111 111.222e111 1e1 1d1 `askd@#$53da\x01\x02\xCCs` "
  //  "SELECT select SeLeCt SELECTION");
  
  //h_file= ::CreateFile("d:/test 20051018 2143.sql", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

  //long t1= ::GetTickCount();
  //yylex(0);
  //printf("%d\n", ::GetTickCount() - t1);

  //::CloseHandle(h_file);
}

void LexerTest::sqlTest()
{
  //char *yylval;
  //int token;
  //lex_input_stream = new std::istringstream("select a from tbl ");
  //token= yylex(&yylval);
  //token= yylex(&yylval);
  //token= yylex(&yylval);
  //token= yylex(&yylval);
  //token= yylex(&yylval);
  //std::ifstream s("test1.sql");
  //std::ifstream s("test2.sql");
}

int stmt_parser_cb(const char *sql, void *user_data)
{
  printf("stmt: '%s'\n", sql);
  return 0;
}

void LexerTest::stmtscanTest()
{
  //const char *c;
  //c= "CREATE DATABASE IF NOT EXISTS `i-flow_dev` CHARACTER SET latin1 COLLATE latin1_swedish_ci;";
  //myx_process_sql_statements(c, stmt_parser_cb, NULL, MYX_SPM_NORMAL_MODE);
  //c= "select * from a; select * from b; insert into c values (1,2,3); /* comment ; */; ;";
  //myx_process_sql_statements(c, stmt_parser_cb, NULL, MYX_SPM_NORMAL_MODE);
  //c= "select * from a; delimiter %%%\n delimiter $$$%%%\n\r select * from a $$$ select ";
  //myx_process_sql_statements(c, stmt_parser_cb, NULL, MYX_SPM_NORMAL_MODE);
  //c= "select * from a; # comment ; \n\r select * from b";
  //myx_process_sql_statements(c, stmt_parser_cb, NULL, MYX_SPM_NORMAL_MODE);
  //c= "/* comment ; ";
  //myx_process_sql_statements(c, stmt_parser_cb, NULL, MYX_SPM_NORMAL_MODE);
  //c= "\" string ";
  //myx_process_sql_statements(c, stmt_parser_cb, NULL, MYX_SPM_NORMAL_MODE);
  //c= "\' string ";
  //myx_process_sql_statements(c, stmt_parser_cb, NULL, MYX_SPM_NORMAL_MODE);
  //c= "-- comment ; something ";
  //myx_process_sql_statements(c, stmt_parser_cb, NULL, MYX_SPM_NORMAL_MODE);
  //c= "# comment ; something ";
  //myx_process_sql_statements(c, stmt_parser_cb, NULL, MYX_SPM_NORMAL_MODE);
  //c= "select * from a; // blablabla ";
  //myx_process_sql_statements(c, stmt_parser_cb, NULL, MYX_SPM_DELIMS_REQUIRED);
  //c= "# comment ; something ";
  //myx_process_sql_statements(c, stmt_parser_cb, NULL, MYX_SPM_DELIMS_REQUIRED); //should _not_ call cb
  //c= "DELIMITER | CREATE TRIGGER t1 BEFORE UPDATE ON t1 BEGIN END| DELIMITER ;";
  //myx_process_sql_statements(c, stmt_parser_cb, NULL, MYX_SPM_NORMAL_MODE);

  // crash test
  //myx_process_sql_statements_from_file("C:\\test 20051018 2143.sql", stmt_parser_cb, NULL, MYX_SPM_DELIMS_REQUIRED);

  // /*! */ test
  //myx_process_sql_statements_from_file("c:/co/qqq.sql", stmt_parser_cb, NULL, MYX_SPM_DELIMS_REQUIRED);

  //c= "";

  //myx_process_sql_statements(c, stmt_parser_cb, NULL, MYX_SPM_NORMAL_MODE);

}

void LexerTest::basicParserTest()
{
  //"select a into outfile \"file.txt\" fields terminated by \",\" optionally enclosed by \"\\\"\" lines starting by \"x\" terminated by \"y\" from tbl1 inner join tbl2 on a = b inner join tbl3 on b = c where x = b group by x having x = z order by z asc, q desc limit 1, 2"
  //"select a from tbl where x in (1,2,3)"
  //"select a from tbl where x not in (select b from tbl where y in (select c from tbl where z in (a,b,c)))"
  //"select 1+2*3*(4-5)"
  //"select * from a where x like y"
  //"select * from a union select * from b union select * from c"
  //"select tbl1.*, tbl2.* from tbl1 a inner join tbl2 b"
  //"insert into q (a,b,c) values (\"aaa\", 123, \"456\")"
  //"insert into tbl (a,b,c) values (1,2,3) on duplicate key update c=values(a)+values(b)"
  //"delete from tbl"
  //"delete from tbl1 where x = y limit 3"
  //"update tbl set a=x, b=y where c=1"
  //myx_set_parser_source("create table t1 (id int)");
  //myx_set_parser_source("use `schema_name`");
  
  //myx_set_parser_source("create table myschema.mytable (mykey int(10) unsigned NOT NULL auto_increment, myf float(10,2) default null, myf float(9), mye enum(\"val1\", \"val2\", \"val3\"), mytext varchar(45) charset latin1 binary NOT NULL default \"\" collate ddd, mychar char(10) ascii, PRIMARY KEY  (mykey), key key1 (mykey(10) asc, mykey(20) desc), unique key using btree (mykey) ) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DEFAULT AUTO_INCREMENT=2 COMMENT = \"comment string\"");
  //myx_set_parser_source("create table mytable (mykey int(10) NOT NULL primary key)");
  //myx_set_parser_source("create table t2 (k int(10) default NULL, constraint con1 foreign key fk1 (k) references t (i) match full on delete set null on update set null)");
  
  //myx_set_parser_source("create table mytable (mykey int(10) unsigned NOT NULL auto_increment, mytext varchar(45) NOT NULL default \"\", PRIMARY KEY  (mykey) ) ENGINE=InnoDB DEFAULT CHARSET=latin1 ROW_FORMAT=DEFAULT AUTO_INCREMENT=2");
  //myx_set_parser_source("drop tables t1, t2, t3 cascade");
  //myx_set_parser_source("create database if not exists db1");
  //myx_set_parser_source("create fulltext index idx1 using btree on tbl1 (k1, k2, k3)");
  //myx_set_parser_source("create view v1 as select * from t1 where x=y with cascaded check option");
  //myx_set_parser_source("create user u1 identified by \"p1\"");
  //myx_set_parser_source("drop index idx1 on tbl1");
  //myx_set_parser_source("drop database db1");
  //myx_set_parser_source("create procedure p1 (in arg1 int(10), out arg2 int(20)) language sql deterministic begin select * into arg2 from tbl; end");
  //myx_set_parser_source("create function f1 (arg1 int(10), arg2 int(20)) returns int(20) return 1");
  //myx_set_parser_source("ALTER TABLE t2 DROP COLUMN c, DROP COLUMN d");
  //myx_set_parser_source("ALTER TABLE t1 CHANGE a b INTEGER");
  //myx_set_parser_source("ALTER TABLE t1 CHANGE b b BIGINT NOT NULL");
  //myx_set_parser_source("ALTER TABLE tbl_name CONVERT TO CHARACTER SET charset_name");
  //myx_set_parser_source("ALTER TABLE tbl_name DISCARD TABLESPACE");
  //myx_set_parser_source("SET sort_buffer_size=10000");
  //myx_set_parser_source("SET @@local.sort_buffer_size=10000");
  //myx_set_parser_source("SET GLOBAL sort_buffer_size=1000000, SESSION sort_buffer_size=1000000");
  //myx_set_parser_source("ANALYZE NO_WRITE_TO_BINLOG TABLE t2");
  //myx_set_parser_source("BACKUP TABLE t2 TO \"/path/to/backup/directory\"");
  //myx_set_parser_source("CALL sp_name(p1, p2, p3)");
  //myx_set_parser_source("CHANGE MASTER TO MASTER_PASSWORD=\"new3cret\"");
  //myx_set_parser_source("CHECK TABLE tbl_name QUICK");
  //myx_set_parser_source("COMMIT NO RELEASE");
  //myx_set_parser_source("DESC SELECT * FROM t4");
  //myx_set_parser_source("DO a = 1, b = 2");
  //myx_set_parser_source("EXECUTE stmt1 USING @a, @b");
  //myx_set_parser_source("flush privileges");
  //myx_set_parser_source("GRANT ALL ON test.* TO \"\"@\"localhost\"");
  //myx_set_parser_source("HANDLER tbl_name READ `PRIMARY` > (a, b, c)");
  //myx_set_parser_source("KILL a");
  //myx_set_parser_source("LOAD DATA INFILE \"data.txt\" INTO TABLE db2.my_table");
  //myx_set_parser_source("LOCK TABLE t WRITE, t AS t1 WRITE");
  //myx_set_parser_source("CACHE INDEX t1, t2, t3 IN hot_cache");
  //myx_set_parser_source("LOAD INDEX INTO CACHE t1, t2 IGNORE LEAVES");
  //myx_set_parser_source("PREPARE stmt1 FROM \"SELECT * FROM t1\"");
  //myx_set_parser_source("PURGE MASTER LOGS TO \"mysql-bin.010\"");
  //myx_set_parser_source("RENAME USER old_user TO new_user");
  //myx_set_parser_source("repair table t2 extended");
  //myx_set_parser_source("reset master");
  //myx_set_parser_source("REVOKE ALL PRIVILEGES, GRANT OPTION FROM user ");
  //myx_set_parser_source("ROLLBACK TO SAVEPOINT identifier");
  //myx_set_parser_source("SHOW BINARY LOGS");
  //myx_set_parser_source("STOP SLAVE");
  //myx_set_parser_source("XA ROLLBACK \"xid\"");
 
  //myx_set_parser_source("DROP TABLE IF EXISTS `db1`.`customer's orders`");

  //myx_set_parser_source("CREATE TABLE `db1`.`customer's orders` ("
  //  "`Level` VARCHAR(6) NOT NULL,"
  //  "`LevelDescription` VARCHAR(255) NULL,"
  //  "`LevelRank` VARCHAR(6) NULL,"
  //  "`LevelText` LONGTEXT NULL,"
  //  "PRIMARY KEY (`Level`)) ENGINE = INNODB");

  //myx_set_parser_input(new std::ifstream("test.sql"));

  myx_parse();
  myx_free_parser_source();
  std::ofstream("treedump.xml") << *static_cast<SqlAstNode *>(myx_get_parser_tree());
}

void LexerTest::fileParse(const char* fileName)
{
  DWORD start = GetTickCount();

  std::istream* stream = new std::ifstream(fileName);
  //lex_input_stream->rdbuf()->pubsetbuf(new char[10*1024*1024], 10*1024*1024);  myx_parse();
  myx_set_parser_input(stream);
  myx_parse();
  void* tree = myx_get_parser_tree();
  if (tree)
    std::ofstream("treedump.xml") << *static_cast<SqlAstNode *>(tree);
  printf("Needed time: %d(ms)\n", ::GetTickCount() - start);
}

void LexerTest::stringParse(const char* str)
{
  std::istream* stream = new std::istringstream(str);
  myx_set_parser_input(stream);
  myx_parse();
  void* tree = myx_get_parser_tree();
  if (tree)
    std::ofstream("treedump.xml") << *static_cast<SqlAstNode *>(tree);
}