#ifndef _DB_MYSQL_PUBLIC_INTERFACE_H_
#define _DB_MYSQL_PUBLIC_INTERFACE_H_

// STATIC_MYSQLMODULEDBMYSQL is used for tut tests, which
// links these classes statically

#if defined(_WIN32) //&& !defined(STATIC_MYSQLMODULEDBMYSQL)

#ifdef MYSQLMODULEDBMYSQL_EXPORTS
#define MYSQLMODULEDBMYSQL_PUBLIC_FUNC __declspec(dllexport)
#else
#define MYSQLMODULEDBMYSQL_PUBLIC_FUNC __declspec(dllimport)
#endif

#else
#define MYSQLMODULEDBMYSQL_PUBLIC_FUNC
#endif

#endif // _DB_MYSQL_PUBLIC_INTERFACE_H_
