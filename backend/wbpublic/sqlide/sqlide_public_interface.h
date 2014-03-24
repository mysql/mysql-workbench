#ifndef _SQLIDE_PUBLIC_INTERFACE_H_
#define _SQLIDE_PUBLIC_INTERFACE_H_

#if defined(__WIN__) || defined(_WIN32) || defined(_WIN64)

#ifdef SQLIDE_EXPORTS
#define SQLIDE_PUBLIC_FUNC __declspec(dllexport)
#else
#define SQLIDE_PUBLIC_FUNC __declspec(dllimport)
#endif

#else
#define SQLIDE_PUBLIC_FUNC
#endif

#endif // _SQLIDE_PUBLIC_INTERFACE_H_
