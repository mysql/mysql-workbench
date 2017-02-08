#ifndef __WB_BACKEND_PUBLIC_INTERFACE_H__
#define __WB_BACKEND_PUBLIC_INTERFACE_H__

#if defined(__WIN__) || defined(_WIN32) || defined(_WIN64)

#ifdef MYSQLWBBACKEND_EXPORTS
#define MYSQLWBBACKEND_PUBLIC_FUNC __declspec(dllexport)
#else
#define MYSQLWBBACKEND_PUBLIC_FUNC __declspec(dllimport)
#endif

#else
#define MYSQLWBBACKEND_PUBLIC_FUNC
#endif

#endif // __WB_BACKEND_PUBLIC_INTERFACE_H__
