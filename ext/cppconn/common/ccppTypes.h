/*
  Some common definitions and inclusions for C/C++.
  I hope it will be so :) Used in tests only so far
*/

#ifndef __C_CPP_TYPES_H_
#define __C_CPP_TYPES_H_

#if defined(_WIN32) || defined(_WIN64)
/* MySQL 5.1 might have defined it before in include/config-win.h */
#ifdef strncasecmp
#undef strncasecmp // We redefine the macro to avoid warnings about using an depricated POSIX name.
#endif 

#define strncasecmp(s1,s2,n) _strnicmp(s1,s2,n)

#ifndef atoll
#define atoll(x) _atoi64((x))
#endif
#else
#include <string.h>
#endif

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <iostream>
#include <list>
/*#include <locale>*/

/*----------------------------------------------------------------------------
ci_char_traits : Case-insensitive char traits.
Taken from : http://gcc.gnu.org/onlinedocs/libstdc++/21_strings/gotw29a.txt
and adapted as template for both 'char' and 'wchar_t' types.
----------------------------------------------------------------------------*/
template <typename charT> struct ci_char_traits
: public std::char_traits<charT>
// just inherit all the other functions
//  that we don't need to override
{
  static bool eq (charT c1, charT c2)
  {
    return std::tolower(c1) == std::tolower(c2);
  }

  static bool ne (charT c1, charT c2)
  {
    return std::tolower(c1) != std::tolower(c2);
  }

  static bool lt (charT c1, charT c2)
  {
    return std::tolower(c1) < std::tolower(c2);
  }

  static int compare (const charT* s1, const charT* s2, size_t n)
  {
    return strncasecmp(s1, s2, n);
  }

  static const charT* find (const charT* s, int n, charT a)
  {
    while (n-- > 0 && std::tolower(*s) != std::tolower(a))
    {
      ++s;
    }
    return (n >= 0 ? s : 0);
  }
};

//Some stubs for unicode use. Following usual win scheme
#ifdef UNICODE_32BIT
typedef std::basic_string
<wchar_t,
std::char_traits<wchar_t>,
std::allocator<wchar_t> >
String;

#ifndef _T
#define _T(strConst) L ## strConst
#endif

#else
typedef std::basic_string
<char,
std::char_traits<char>,
std::allocator<char> >
String;

typedef std::basic_string
<char,
ci_char_traits<char>,
std::allocator<char> >
ciString;

#ifndef _T
#define _T(strConst) strConst
#endif

#endif

typedef std::vector<String> List;
typedef List::iterator Iterator;

typedef std::map<String, String> Properties;
typedef Properties::iterator  PropsIterator;

#endif /* __C_CPP_TYPES_H_ */
