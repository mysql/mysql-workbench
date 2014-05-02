#ifndef __MDC_CANVAS_PUBLIC_H__
#define __MDC_CANVAS_PUBLIC_H__

#if defined(__WIN__) || defined(_WIN32) || defined(_WIN64)

#ifdef MYSQLCANVAS_EXPORTS
#define MYSQLCANVAS_PUBLIC_FUNC __declspec(dllexport)
#else
#define MYSQLCANVAS_PUBLIC_FUNC __declspec(dllimport)
#endif

#else
#define MYSQLCANVAS_PUBLIC_FUNC
#endif 

#endif // __MDC_CANVAS_PUBLIC_H__

