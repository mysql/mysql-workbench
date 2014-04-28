#ifndef _WB_EDITOR_BACKEND_PUBLIC_INTERFACE_H_
#define _WB_EDITOR_BACKEND_PUBLIC_INTERFACE_H_

#if defined(__WIN__) || defined(_WIN32) || defined(_WIN64)

#ifdef WBEDITOR_BACKEND_EXPORTS
#define WBEDITOR_BACKEND_PUBLIC_FUNC __declspec(dllexport)
#else
#define WBEDITOR_BACKEND_PUBLIC_FUNC __declspec(dllimport)
#endif

#else
#define WBEDITOR_BACKEND_PUBLIC_FUNC
#endif

#endif // _WB_EDITOR_BACKEND_PUBLIC_INTERFACE_H_
