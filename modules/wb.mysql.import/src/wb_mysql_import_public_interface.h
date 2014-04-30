#ifndef _WB_MYSQL_IMPORT_PUBLIC_INTERFACE_H_
#define _WB_MYSQL_IMPORT_PUBLIC_INTERFACE_H_

// STATIC_WB_MYSQL_WBM_IMPORT is used for tut tests, which
// links these classes statically

#if defined(_WIN32) && !defined(STATIC_WB_MODULE_IMPORT)

#ifdef WB_MYSQL_IMPORT_WBM_EXPORTS
#define WB_MYSQL_IMPORT_WBM_PUBLIC_FUNC __declspec(dllexport)
#else
#define WB_MYSQL_IMPORT_WBM_PUBLIC_FUNC __declspec(dllimport)
#endif

#else
#define WB_MYSQL_IMPORT_WBM_PUBLIC_FUNC
#endif

#endif // _WB_MYSQL_IMPORT_PUBLIC_INTERFACE_H_

