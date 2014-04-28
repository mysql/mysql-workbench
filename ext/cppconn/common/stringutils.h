#ifndef __CCPP_STRINGUTILS_H
#define __CCPP_STRINGUTILS_H

#include "ccppTypes.h"
#include <sstream>

namespace StringUtils
{
  unsigned  split     ( List &          list
                      , const String &  str
                      , const String &  delim
                      , bool            trimItems/*includeEmpty*/ = true );

  /* Unlike Java's trim, this trims only blank spaces */
  String    trim      ( const String & victim );

  int       toInt     ( const String & str, bool isNull = false );
  bool      toBoolean ( const String & str, bool isNull = false );
  long long toLong    ( const String & str, bool isNull = false );
  float     toFloat   ( const String & str, bool isNull = false );
  double    toDouble  ( const String & str, bool isNull = false );

  const String &  defaultIfEmpty  ( const String & str, const String & defStr );
}

#endif
