/*
* Copyright (c) 2004, 2014, Oracle and/or its affiliates. All rights reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; version 2 of the
* License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301  USA
*/

#ifndef _STREAM_CONV_H_
#define _STREAM_CONV_H_

#include <sstream>


/*
DESCRIPTION:
converts input stream using custom conv function & writes to output stream.
*/
template<typename T1, typename T2>
bool stream_conv(std::istream& is,
                 std::ostream& os,
                 T2&(*conv_func)(const T1&, T2&))
{
  T1 ival;
  T2 oval;

  while (os && (is >> ival))
    os << conv_func(ival, oval);

  return os.fail();
}


/*
class representing text-based HEX value
*/
template<size_t N>
class Hex_string
{
public:
  static size_t size() { return N; }
  const char* value() const { return val; }
  std::istream& read(std::istream& is) { is.read(val, N); return is; }
private:
  char val[N];
};

template<typename T>
inline std::istream& operator>>(std::istream& is, T& v)
{
  return v.read(is);
}


/*
translation from text-based hex value to raw form of data
*/
template<size_t N, typename T2>
inline T2& unhex(const Hex_string<N>& ival, T2& oval)
{
  typedef Hex_string<N> T1;
  oval= 0;
  const char* buf= ival.value();
  for (size_t n= 0; n < sizeof(T1)/sizeof(T2); ++n)
  {
    T2 v= buf[n];
    v-= (v < 'A') ? '0' : ('A' - 10);
    oval+= v << (4*(sizeof(T1)/sizeof(T2)-n-1));
  }
  return oval;
}


#endif // _STREAM_CONV_H_
