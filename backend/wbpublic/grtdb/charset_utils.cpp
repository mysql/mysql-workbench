/* 
 * Copyright (c) 2007, 2010, Oracle and/or its affiliates. All rights reserved.
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

#include "stdafx.h"

#include <algorithm>
#include <string>
#include <map>

#include "base/string_utilities.h"
#include "charset_utils.h"

#define ARR_CAPACITY(arr) (sizeof(arr)/sizeof(arr[0]))

const std::string & get_cs_def_collation(std::string cs_name)
{
  static std::string empty_cs_collation_name;
  static std::map<std::string, std::string> def_collations;
  if (def_collations.empty())
  {
    const char *def_collations_arr[][2]= {
      { "armscii8", "armscii8_general_ci" },
      { "ascii", "ascii_general_ci" },
      { "big5", "big5_chinese_ci" },
      { "binary", "binary" },
      { "cp1250", "cp1250_general_ci" },
      { "cp1251", "cp1251_general_ci" },
      { "cp1256", "cp1256_general_ci" },
      { "cp1257", "cp1257_general_ci" },
      { "cp850", "cp850_general_ci" },
      { "cp852", "cp852_general_ci" },
      { "cp866", "cp866_general_ci" },
      { "cp932", "cp932_japanese_ci" },
      { "dec8", "dec8_swedish_ci" },
      { "eucjpms", "eucjpms_japanese_ci" },
      { "euckr", "euckr_korean_ci" },
      { "gb2312", "gb2312_chinese_ci" },
      { "gbk", "gbk_chinese_ci" },
      { "geostd8", "geostd8_general_ci" },
      { "greek", "greek_general_ci" },
      { "hebrew", "hebrew_general_ci" },
      { "hp8", "hp8_english_ci" },
      { "keybcs2", "keybcs2_general_ci" },
      { "koi8r", "koi8r_general_ci" },
      { "koi8u", "koi8u_general_ci" },
      { "latin1", "latin1_swedish_ci" },
      { "latin2", "latin2_general_ci" },
      { "latin5", "latin5_turkish_ci" },
      { "latin7", "latin7_general_ci" },
      { "macce", "macce_general_ci" },
      { "macroman", "macroman_general_ci" },
      { "sjis", "sjis_japanese_ci" },
      { "swe7", "swe7_swedish_ci" },
      { "tis620", "tis620_thai_ci" },
      { "ucs2", "ucs2_general_ci" },
      { "ujis", "ujis_japanese_ci" },
      { "utf8", "utf8_general_ci" }
    };

    for (size_t n= 0, count= ARR_CAPACITY(def_collations_arr); n < count; ++n)
      def_collations[def_collations_arr[n][0]]= def_collations_arr[n][1];
  }

  cs_name = base::tolower(cs_name);
  std::map<std::string, std::string>::iterator i= def_collations.find(cs_name);
  if (def_collations.end() != i)
    return i->second;

  return empty_cs_collation_name;
}


const std::string & get_collation_cs(std::string collation_name)
{
  static std::string empty_cs_name;
  static std::map<std::string, std::string> collations;
  if (collations.empty())
  {
    const char *collations_arr[][2]= {
      { "armscii8_bin", "armscii8" },
      { "armscii8_general_ci", "armscii8" },
      { "ascii_bin", "ascii" },
      { "ascii_general_ci", "ascii" },
      { "big5_bin", "big5" },
      { "big5_chinese_ci", "big5" },
      { "binary", "binary" },
      { "cp1250_bin", "cp1250" },
      { "cp1250_croatian_ci", "cp1250" },
      { "cp1250_czech_cs", "cp1250" },
      { "cp1250_general_ci", "cp1250" },
      { "cp1250_polish_ci", "cp1250" },
      { "cp1251_bin", "cp1251" },
      { "cp1251_bulgarian_ci", "cp1251" },
      { "cp1251_general_ci", "cp1251" },
      { "cp1251_general_cs", "cp1251" },
      { "cp1251_ukrainian_ci", "cp1251" },
      { "cp1256_bin", "cp1256" },
      { "cp1256_general_ci", "cp1256" },
      { "cp1257_bin", "cp1257" },
      { "cp1257_general_ci", "cp1257" },
      { "cp1257_lithuanian_ci", "cp1257" },
      { "cp850_bin", "cp850" },
      { "cp850_general_ci", "cp850" },
      { "cp852_bin", "cp852" },
      { "cp852_general_ci", "cp852" },
      { "cp866_bin", "cp866" },
      { "cp866_general_ci", "cp866" },
      { "cp932_bin", "cp932" },
      { "cp932_japanese_ci", "cp932" },
      { "dec8_bin", "dec8" },
      { "dec8_swedish_ci", "dec8" },
      { "eucjpms_bin", "eucjpms" },
      { "eucjpms_japanese_ci", "eucjpms" },
      { "euckr_bin", "euckr" },
      { "euckr_korean_ci", "euckr" },
      { "gb2312_bin", "gb2312" },
      { "gb2312_chinese_ci", "gb2312" },
      { "gbk_bin", "gbk" },
      { "gbk_chinese_ci", "gbk" },
      { "geostd8_bin", "geostd8" },
      { "geostd8_general_ci", "geostd8" },
      { "greek_bin", "greek" },
      { "greek_general_ci", "greek" },
      { "hebrew_bin", "hebrew" },
      { "hebrew_general_ci", "hebrew" },
      { "hp8_bin", "hp8" },
      { "hp8_english_ci", "hp8" },
      { "keybcs2_bin", "keybcs2" },
      { "keybcs2_general_ci", "keybcs2" },
      { "koi8r_bin", "koi8r" },
      { "koi8r_general_ci", "koi8r" },
      { "koi8u_bin", "koi8u" },
      { "koi8u_general_ci", "koi8u" },
      { "latin1_bin", "latin1" },
      { "latin1_danish_ci", "latin1" },
      { "latin1_general_ci", "latin1" },
      { "latin1_general_cs", "latin1" },
      { "latin1_german1_ci", "latin1" },
      { "latin1_german2_ci", "latin1" },
      { "latin1_spanish_ci", "latin1" },
      { "latin1_swedish_ci", "latin1" },
      { "latin2_bin", "latin2" },
      { "latin2_croatian_ci", "latin2" },
      { "latin2_czech_cs", "latin2" },
      { "latin2_general_ci", "latin2" },
      { "latin2_hungarian_ci", "latin2" },
      { "latin5_bin", "latin5" },
      { "latin5_turkish_ci", "latin5" },
      { "latin7_bin", "latin7" },
      { "latin7_estonian_cs", "latin7" },
      { "latin7_general_ci", "latin7" },
      { "latin7_general_cs", "latin7" },
      { "macce_bin", "macce" },
      { "macce_general_ci", "macce" },
      { "macroman_bin", "macroman" },
      { "macroman_general_ci", "macroman" },
      { "sjis_bin", "sjis" },
      { "sjis_japanese_ci", "sjis" },
      { "swe7_bin", "swe7" },
      { "swe7_swedish_ci", "swe7" },
      { "tis620_bin", "tis620" },
      { "tis620_thai_ci", "tis620" },
      { "ucs2_bin", "ucs2" },
      { "ucs2_czech_ci", "ucs2" },
      { "ucs2_danish_ci", "ucs2" },
      { "ucs2_esperanto_ci", "ucs2" },
      { "ucs2_estonian_ci", "ucs2" },
      { "ucs2_general_ci", "ucs2" },
      { "ucs2_hungarian_ci", "ucs2" },
      { "ucs2_icelandic_ci", "ucs2" },
      { "ucs2_latvian_ci", "ucs2" },
      { "ucs2_lithuanian_ci", "ucs2" },
      { "ucs2_persian_ci", "ucs2" },
      { "ucs2_polish_ci", "ucs2" },
      { "ucs2_roman_ci", "ucs2" },
      { "ucs2_romanian_ci", "ucs2" },
      { "ucs2_slovak_ci", "ucs2" },
      { "ucs2_slovenian_ci", "ucs2" },
      { "ucs2_spanish_ci", "ucs2" },
      { "ucs2_spanish2_ci", "ucs2" },
      { "ucs2_swedish_ci", "ucs2" },
      { "ucs2_turkish_ci", "ucs2" },
      { "ucs2_unicode_ci", "ucs2" },
      { "ujis_bin", "ujis" },
      { "ujis_japanese_ci", "ujis" },
      { "utf8_bin", "utf8" },
      { "utf8_czech_ci", "utf8" },
      { "utf8_danish_ci", "utf8" },
      { "utf8_esperanto_ci", "utf8" },
      { "utf8_estonian_ci", "utf8" },
      { "utf8_general_ci", "utf8" },
      { "utf8_hungarian_ci", "utf8" },
      { "utf8_icelandic_ci", "utf8" },
      { "utf8_latvian_ci", "utf8" },
      { "utf8_lithuanian_ci", "utf8" },
      { "utf8_persian_ci", "utf8" },
      { "utf8_polish_ci", "utf8" },
      { "utf8_roman_ci", "utf8" },
      { "utf8_romanian_ci", "utf8" },
      { "utf8_slovak_ci", "utf8" },
      { "utf8_slovenian_ci", "utf8" },
      { "utf8_spanish_ci", "utf8" },
      { "utf8_spanish2_ci", "utf8" },
      { "utf8_swedish_ci", "utf8" },
      { "utf8_turkish_ci", "utf8" },
      { "utf8_unicode_ci", "utf8" },
    };

    for (size_t n= 0, count= ARR_CAPACITY(collations_arr); n < count; ++n)
      collations[collations_arr[n][0]]= collations_arr[n][1];
  }

  collation_name = base::tolower(collation_name);
  std::map<std::string, std::string>::iterator i= collations.find(collation_name);
  if (collations.end() != i)
    return i->second;

  return empty_cs_name;
}
