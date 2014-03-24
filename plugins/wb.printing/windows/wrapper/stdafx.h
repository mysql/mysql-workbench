#ifndef _STDAFX_H_
#define _STDAFX_H_

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>

#pragma warning(disable: 4793)  // 'vararg' causes native code generation

#include <algorithm>

#endif // _WIN32

#endif // _STDAFX_H_