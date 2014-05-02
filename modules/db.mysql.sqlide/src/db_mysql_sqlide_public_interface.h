#ifndef _DB_MYSQL_SQLIDE_PUBLIC_INTERFACE_H_
#define _DB_MYSQL_SQLIDE_PUBLIC_INTERFACE_H_

// STATIC_WB_MYSQL_WBM_IMPORT is used for tut tests, which
// links these classes statically

#if defined(_WIN32) && !defined(STATIC_WB_MODULE_IMPORT)

#ifdef DB_MYSQL_SQLIDE_EXPORTS
#define DB_MYSQL_SQLIDE_PUBLIC_FUNC __declspec(dllexport)
#else
#define DB_MYSQL_SQLIDE_PUBLIC_FUNC __declspec(dllimport)
#endif

#else
#define DB_MYSQL_SQLIDE_PUBLIC_FUNC
#endif

#endif // _DB_MYSQL_SQLIDE_PUBLIC_INTERFACE_H_

