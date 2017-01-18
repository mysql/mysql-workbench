#ifndef __GRTDB_BACKEND_PUBLIC_INTERFACE_H__
#define __GRTDB_BACKEND_PUBLIC_INTERFACE_H__

#if defined(__WIN__) || defined(_WIN32) || defined(_WIN64)

#ifdef MYSQLGRTDBBACKEND_EXPORTS
#define MYSQLGRTDBBACKEND_PUBLIC_FUNC __declspec(dllexport)
#else
#define MYSQLGRTDBBACKEND_PUBLIC_FUNC __declspec(dllimport)
#endif

#else
#define MYSQLGRTDBBACKEND_PUBLIC_FUNC
#endif

#endif // __GRTDB_BACKEND_PUBLIC_INTERFACE_H__
