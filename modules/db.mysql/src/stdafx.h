#ifndef _STDAFX_H_
#define _STDAFX_H_

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>

#include <glib.h>
#include <cctype>
#include <algorithm>
#include <sstream>
#include <map>
#include <set>
#include <vector>

#else
#include <stdio.h>

inline char* itoa(const int value, char* buf, const int /*base*/)
{
  sprintf(buf, "%i", value);
  return buf;
}

#define stricmp strcasecmp

#endif 

#include <pcre.h>

#endif // _STDAFX_H_
