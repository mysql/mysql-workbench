#ifndef _WB_MODEL_PUBLIC_INTERFACE_H_
#define _WB_MODEL_PUBLIC_INTERFACE_H_

#ifdef _WIN32

#ifdef WB_MODEL_WBM_EXPORTS
#define WB_MODEL_WBM_PUBLIC_FUNC __declspec(dllexport)
#else
#define WB_MODEL_WBM_PUBLIC_FUNC __declspec(dllimport)
#endif

#else
#define WB_MODEL_WBM_PUBLIC_FUNC
#endif

#endif // _WB_MODEL_PUBLIC_INTERFACE_H_
