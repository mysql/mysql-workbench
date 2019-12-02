/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation. The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

static const std::set<std::string> systemFunctions55 = {
  "abs", "acos", "adddate", "addtime", "aes_decrypt", "aes_encrypt", "area", "asbinary", "ascii", "asin", "astext", "atan", "atan2", "atan", "avg",
  "benchmark", "bin", "bit_and", "bit_count", "bit_length", "bit_or", "bit_xor", "boundary",
  "cast", "ceil", "ceiling", "char_length", "char", "character_length", "charset", "coalesce", "coercibility", "collation", "compress", "concat_ws", "concat", "connection_id",
  "contains", "conv", "convert_tz", "convert", "cos", "cot", "count", "crc32", "crosses", "curdate", "current_date", "current_time", "current_timestamp",
  "current_user", "curtime",
  "database", "date_add", "date_format", "date_sub", "date", "datediff", "day", "dayname", "dayofmonth", "dayofweek", "dayofyear", "decode", "default", "degrees", "des_decrypt",
  "des_encrypt", "dimension", "disjoint",
  "elt", "encode", "encrypt", "endpoint", "envelope", "equals", "exp", "export_set", "exteriorring", "extract", "extractvalue",
  "field", "find_in_set", "floor", "format", "found_rows", "from_days", "from_unixtime",
  "geomcollfromtext", "geomcollfromwkb", "geometrycollection", "geometryn", "geometrytype", "geomfromtext", "geomfromwkb", "get_format", "get_lock", "glength",
  "greatest", "group_concat",
  "hex", "hour",
  "if", "ifnull", "inet_aton", "inet_ntoa", "insert", "instr", "interiorringn", "intersects", "interval", "is_free_lock", "is_used_lock", "isclosed", "isempty", "isnull",
  "issimple",
  "last_insert_id", "lcase", "least", "left", "length", "linefromtext", "linefromwkb", "linestring", "ln", "load_file", "localtime", "localtimestamp", "locate", "log10",
  "log2", "log", "lower", "lpad", "ltrim", "make_set",
  "makedate", "maketime", "master_pos_wait", "max", "mbrcontains", "mbrdisjoint", "mbrequal", "mbrintersects", "mbroverlaps", "mbrtouches", "mbrwithin", "md5",
  "microsecond", "mid", "min", "minute", "mlinefromtext", "mlinefromwkb", "mod", "month", "monthname", "mpointfromtext", "mpointfromwkb", "mpolyfromtext",
  "mpolyfromwkb", "multilinestring", "multipoint", "multipolygon",
  "name_const", "now", "nullif", "numgeometries", "numinteriorrings", "numpoints",
  "oct", "octet_length", "old_password", "ord", "overlaps", "password",
  "period_add", "period_diff", "pi", "point", "pointfromtext", "pointfromwkb", "pointn", "polyfromtext", "polyfromwkb", "polygon", "position", "pow", "power",
  "quarter", "quote",
  "radians", "rand", "release_lock", "repeat", "replace", "reverse", "right", "round", "row_count", "rpad", "rtrim",
  "schema", "sec_to_time", "second", "session_user", "sha", "sha1", "sha2", "sign", "sin", "sleep", "soundex", "space", "sqrt", "srid", "startpoint", "std", "stddev_pop", "stddev_samp",
  "stddev", "str_to_date", "strcmp", "subdate", "substr", "substring_index", "substring", "subtime", "sum", "sysdate", "system_user",
  "tan", "time_format", "time_to_sec", "time", "timediff", "timestamp", "timestampadd", "timestampdiff", "to_days", "to_seconds", "touches", "trim", "truncate",
  "ucase", "uncompress", "uncompressed_length", "unhex", "unix_timestamp", "upper", "user", "utc_date", "utc_time", "utc_timestamp", "updatexml", "uuid", "uuid_short",
  "values", "var_pop", "var_samp", "variance", "version",
  "week", "weekday", "weekofyear", "within",
  "x",
  "y", "year", "yearweek"
};

static const std::set<std::string> systemFunctions56 = {
  "abs", "acos", "adddate", "addtime", "aes_decrypt", "aes_encrypt", "area", "asbinary", "ascii", "asin", "astext", "atan", "atan2", "atan", "avg",
  "benchmark", "bin", "bit_and", "bit_count", "bit_length", "bit_or", "bit_xor", "boundary", "buffer",
  "cast", "ceil", "ceiling", "char_length", "char", "character_length", "charset", "coalesce", "coercibility", "collation", "compress", "concat_ws", "concat", "connection_id",
  "contains", "conv", "convert_tz", "convert", "cos", "cot", "count", "crc32", "crosses", "curdate", "current_date", "current_time", "current_timestamp",
  "current_user", "curtime",
  "database", "date_add", "date_format", "date_sub", "date", "datediff", "day", "dayname", "dayofmonth", "dayofweek", "dayofyear", "decode", "default", "degrees", "des_decrypt",
  "des_encrypt", "dimension", "disjoint",
  "elt", "encode", "encrypt", "endpoint", "envelope", "equals", "exp", "export_set", "exteriorring", "extract", "extractvalue",
  "field", "find_in_set", "floor", "format", "found_rows", "from_base64", "from_days", "from_unixtime",
  "geomcollfromtext", "geomcollfromwkb", "geometrycollection", "geometryn", "geometrytype", "geomfromtext", "geomfromwkb", "get_format", "get_lock", "glength",
  "greatest", "group_concat", "gtid_subset", "gtid_subtract",
  "hex", "hour",
  "if", "ifnull", "inet_aton", "inet_ntoa", "inet6_aton", "inet6_ntoa", "insert", "instr", "interiorringn", "intersects", "interval", "is_free_lock", "ipv4_compat", "is_ipv4_mapped",
  "is_ipv4", "is_ipv6", "is_used_lock", "isclosed", "isempty", "isnull", "issimple",
  "last_insert_id", "lcase", "least", "left", "length", "linefromtext", "linefromwkb", "linestring", "ln", "load_file", "localtime", "localtimestamp", "locate", "log10",
  "log2", "log", "lower", "lpad", "ltrim", "make_set",
  "makedate", "maketime", "master_pos_wait", "max", "mbrcontains", "mbrdisjoint", "mbrequal", "mbrintersects", "mbroverlaps", "mbrtouches", "mbrwithin", "md5",
  "microsecond", "mid", "min", "minute", "mlinefromtext", "mlinefromwkb", "mod", "month", "monthname", "mpointfromtext", "mpointfromwkb", "mpolyfromtext",
  "mpolyfromwkb", "multilinestring", "multipoint", "multipolygon",
  "name_const", "now", "nullif", "numgeometries", "numinteriorrings", "numpoints",
  "oct", "octet_length", "old_password", "ord", "overlaps", "password",
  "period_add", "period_diff", "pi", "point", "pointfromtext", "pointfromwkb", "pointn", "polyfromtext", "polyfromwkb", "polygon", "position", "pow", "power",
  "quarter", "quote",
  "radians", "rand", "random_bytes", "release_lock", "repeat", "replace", "reverse", "right", "round", "row_count", "rpad", "rtrim",
  "schema", "sec_to_time", "second", "session_user", "sha", "sha1", "sha2", "sign", "sin", "sleep", "soundex", "space", "sql_thread_wait_after_gtids", "sqrt", "srid",
  "st_contains", "st_crosses", "st_disjoint", "st_equals", "st_intersects", "st_overlaps", "st_touches", "st_within", "startpoint", "std", "stddev_pop",
  "stddev_samp", "stddev", "str_to_date", "strcmp", "subdate", "substr", "substring_index2", "substring", "subtime", "sum", "sysdate", "system_user",
  "tan", "time_format", "time_to_sec", "time", "timediff", "timestamp", "timestampadd", "timestampdiff", "to_base64", "to_days", "to_seconds", "touches", "trim",
  "truncate", "ucase", "uncompress", "uncompressed_length", "unhex", "unix_timestamp", "upper", "user", "utc_date", "utc_time", "utc_timestamp", "updatexml",
  "uuid", "uuid_short", "validate_password_strength", "values", "var_pop", "var_samp", "variance", "version", "wait_until_sql_thread_after_gtids",
  "week", "weekday", "weekofyear", "weight_string", "within",
  "x",
  "y", "year", "yearweek"
};

static const std::set<std::string> systemFunctions57 = {
  "abs", "acos", "adddate", "addtime", "aes_decrypt", "aes_encrypt", "area", "asbinary", "ascii", "asin", "astext", "atan", "atan2", "atan", "avg",
  "benchmark", "bin", "bit_and", "bit_count", "bit_length", "bit_or", "bit_xor", "boundary", "buffer",
  "cast", "ceil", "ceiling", "char_length", "char", "character_length", "charset", "coalesce", "coercibility", "collation", "compress", "concat_ws", "concat", "connection_id",
  "contains", "conv", "convert_tz", "convert", "cos", "cot", "count", "crc32", "crosses", "curdate", "current_date", "current_time", "current_timestamp",
  "current_user", "curtime",
  "database", "date_add", "date_format", "date_sub", "date", "datediff", "day", "dayname", "dayofmonth", "dayofweek", "dayofyear", "decode", "default", "degrees", "des_decrypt",
  "des_encrypt", "dimension", "disjoint",
  "elt", "encode", "encrypt", "endpoint", "envelope", "equals", "exp", "export_set", "exteriorring", "extract", "extractvalue",
  "field", "find_in_set", "floor", "format", "found_rows", "from_base64", "from_days", "from_unixtime",
  "geomcollfromtext", "geomcollfromwkb", "geometrycollection", "geometryn", "geometrytype", "geomfromtext", "geomfromwkb", "get_format", "get_lock", "glength",
  "greatest", "group_concat", "gtid_subset", "gtid_subtract",
  "hex", "hour",
  "if", "ifnull", "inet_aton", "inet_ntoa", "inet6_aton", "inet6_ntoa", "insert", "instr", "interiorringn", "intersects", "interval", "is_free_lock", "ipv4_compat", "is_ipv4_mapped",
  "is_ipv4", "is_ipv6", "is_used_lock", "isclosed", "isempty", "isnull", "issimple",
  "last_insert_id", "lcase", "least", "left", "length", "linefromtext", "linefromwkb", "linestring", "ln", "load_file", "localtime", "localtimestamp", "locate", "log10",
  "log2", "log", "lower", "lpad", "ltrim", "make_set",
  "makedate", "maketime", "master_pos_wait", "max", "mbrcontains", "mbrdisjoint", "mbrequal", "mbrintersects", "mbroverlaps", "mbrtouches", "mbrwithin", "md5",
  "microsecond", "mid", "min", "minute", "mlinefromtext", "mlinefromwkb", "mod", "month", "monthname", "mpointfromtext", "mpointfromwkb", "mpolyfromtext",
  "mpolyfromwkb", "multilinestring", "multipoint", "multipolygon",
  "name_const", "now", "nullif", "numgeometries", "numinteriorrings", "numpoints",
  "oct", "octet_length", "old_password", "ord", "overlaps", "password",
  "period_add", "period_diff", "pi", "point", "pointfromtext", "pointfromwkb", "pointn", "polyfromtext", "polyfromwkb", "polygon", "position", "pow", "power",
  "quarter", "quote",
  "radians", "rand", "random_bytes", "release_lock", "repeat", "replace", "reverse", "right", "round", "row_count", "rpad", "rtrim",
  "schema", "sec_to_time", "second", "session_user", "sha", "sha1", "sha2", "sign", "sin", "sleep", "soundex", "space", "sqrt", "srid",
  "st_contains", "st_crosses", "st_disjoint", "st_equals", "st_intersects", "st_overlaps", "st_touches", "st_within", "startpoint", "std", "stddev_pop",
  "stddev_samp", "stddev", "str_to_date", "strcmp", "subdate", "substr", "substring_index2", "substring", "subtime", "sum", "sysdate", "system_user",
  "tan", "time_format", "time_to_sec", "time", "timediff", "timestamp", "timestampadd", "timestampdiff", "to_base64", "to_days", "to_seconds", "touches", "trim",
  "truncate", "ucase", "uncompress", "uncompressed_length", "unhex", "unix_timestamp", "upper", "user", "utc_date", "utc_time", "utc_timestamp", "updatexml",
  "uuid", "uuid_short", "validate_password_strength", "values", "var_pop", "var_samp", "variance", "version", "wait_until_sql_thread_after_gtids",
  "week", "weekday", "weekofyear", "weight_string", "within",
  "x",
  "y", "year", "yearweek"
};

std::set<std::string> systemFunctions80 = {
  "abs", "acos", "adddate", "addtime", "aes_decrypt", "aes_encrypt", "area", "asbinary", "ascii", "asin", "astext", "atan", "atan2", "atan", "avg",
  "benchmark", "bin", "bit_and", "bit_count", "bit_length", "bit_or", "bit_xor", "boundary", "buffer",
  "cast", "ceil", "ceiling", "char_length", "char", "character_length", "charset", "coalesce", "coercibility", "collation", "compress", "concat_ws", "concat", "connection_id",
  "contains", "conv", "convert_tz", "convert", "cos", "cot", "count", "crc32", "crosses", "curdate", "current_date", "current_time", "current_timestamp",
  "current_user", "curtime",
  "database", "date_add", "date_format", "date_sub", "date", "datediff", "day", "dayname", "dayofmonth", "dayofweek", "dayofyear", "decode", "default", "degrees", "des_decrypt",
  "des_encrypt", "dimension", "disjoint",
  "elt", "encode", "encrypt", "endpoint", "envelope", "equals", "exp", "export_set", "exteriorring", "extract", "extractvalue",
  "field", "find_in_set", "floor", "format", "found_rows", "from_base64", "from_days", "from_unixtime",
  "geomcollfromtext", "geomcollfromwkb", "geometrycollection", "geometryn", "geometrytype", "geomfromtext", "geomfromwkb", "get_format", "get_lock", "glength",
  "greatest", "group_concat", "gtid_subset", "gtid_subtract",
  "hex", "hour",
  "if", "ifnull", "inet_aton", "inet_ntoa", "inet6_aton", "inet6_ntoa", "insert", "instr", "interiorringn", "intersects", "interval", "is_free_lock", "ipv4_compat", "is_ipv4_mapped",
  "is_ipv4", "is_ipv6", "is_used_lock", "isclosed", "isempty", "isnull", "issimple",
  "last_insert_id", "lcase", "least", "left", "length", "linefromtext", "linefromwkb", "linestring", "ln", "load_file", "localtime", "localtimestamp", "locate", "log10",
  "log2", "log", "lower", "lpad", "ltrim", "make_set",
  "makedate", "maketime", "master_pos_wait", "max", "mbrcontains", "mbrdisjoint", "mbrequal", "mbrintersects", "mbroverlaps", "mbrtouches", "mbrwithin", "md5",
  "microsecond", "mid", "min", "minute", "mlinefromtext", "mlinefromwkb", "mod", "month", "monthname", "mpointfromtext", "mpointfromwkb", "mpolyfromtext",
  "mpolyfromwkb", "multilinestring", "multipoint", "multipolygon",
  "name_const", "now", "nullif", "numgeometries", "numinteriorrings", "numpoints",
  "oct", "octet_length", "old_password", "ord", "overlaps", "password",
  "period_add", "period_diff", "pi", "point", "pointfromtext", "pointfromwkb", "pointn", "polyfromtext", "polyfromwkb", "polygon", "position", "pow", "power",
  "quarter", "quote",
  "radians", "rand", "random_bytes", "release_lock", "repeat", "replace", "reverse", "right", "round", "row_count", "rpad", "rtrim",
  "schema", "sec_to_time", "second", "session_user", "sha", "sha1", "sha2", "sign", "sin", "sleep", "soundex", "space", "sqrt", "srid",
  "st_contains", "st_crosses", "st_disjoint", "st_equals", "st_intersects", "st_overlaps", "st_touches", "st_within", "startpoint", "std", "stddev_pop",
  "stddev_samp", "stddev", "str_to_date", "strcmp", "subdate", "substr", "substring_index2", "substring", "subtime", "sum", "sysdate", "system_user",
  "tan", "time_format", "time_to_sec", "time", "timediff", "timestamp", "timestampadd", "timestampdiff", "to_base64", "to_days", "to_seconds", "touches", "trim",
  "truncate", "ucase", "uncompress", "uncompressed_length", "unhex", "unix_timestamp", "upper", "user", "utc_date", "utc_time", "utc_timestamp", "updatexml",
  "uuid", "uuid_short", "validate_password_strength", "values", "var_pop", "var_samp", "variance", "version", "wait_until_sql_thread_after_gtids",
  "week", "weekday", "weekofyear", "weight_string", "within",
  "x",
  "y", "year", "yearweek"
};
