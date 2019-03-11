/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#ifndef _SQLIDE_GENERICS_H_
#define _SQLIDE_GENERICS_H_

#include "wbpublic_public_interface.h"
#include <sqlite/result.hpp>
#include <sqlite/connection.hpp>
#include <limits>
#include <ctime>
#include <base/string_utilities.h>
#include <sstream>

namespace sqlide {

  using namespace sqlite;

  class WBPUBLICBACKEND_PUBLIC_FUNC VarEq : public boost::static_visitor<bool> {
  public:
    VarEq() {
    }
    template <typename T>
    result_type operator()(const null_t &v1, const T &v2) const {
      return false;
    }
    template <typename T>
    result_type operator()(const T &v1, const null_t &v2) const {
      return false;
    }
    result_type operator()(const null_t &v1, const null_t &v2) const {
      return true;
    }

    template <class T>
    result_type cmp(const unknown_t &v1, const T &v2) const {
      return false;
    }
    template <class T>
    result_type cmp(const T &v1, const unknown_t &v2) const {
      return false;
    }
    result_type operator()(const unknown_t &v1, const unknown_t &v2) const {
      return false;
    }

    template <class T1, class T2>
    result_type operator()(const T1 &v1, const T2 &v2) const {
      return false;
    }
    template <typename T>
    result_type operator()(const T &v1, const T &v2) const {
      return v1 == v2;
    }
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC VarConvBase : public boost::static_visitor<std::string> {
  public:
    VarConvBase() {
      _ss.precision(std::numeric_limits<long double>::digits10);
    }

  protected:
    class StateKeeper {
    public:
      inline StateKeeper(VarConvBase *obj) : _obj(obj) {
      }
      inline ~StateKeeper() {
        _obj->reset();
      }

    private:
      VarConvBase *_obj;
    };
    friend class StateKeeper;
    mutable std::stringstream _ss;
    inline void reset() {
      _ss.str("");
    }
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC VarToStr : public VarConvBase {
  public:
    VarToStr() : VarConvBase(), is_truncation_enabled(false), truncation_threshold(std::string::npos) {
    }
    bool is_truncation_enabled;
    std::string::size_type truncation_threshold;

    result_type operator()(const unknown_t &v) const {
      return "";
    }
    result_type operator()(const null_t &v) const {
      return "";
    }
    result_type operator()(const blob_ref_t &v) const {
      // if (is_truncation_enabled)
      return "...";
      // else
      //  throw std::runtime_error("Can't convert BLOB to string value");
    }

    result_type operator()(const std::string &v) const {
      return (is_truncation_enabled && (truncation_threshold < v.size()))
               ? base::truncate_text(v, (int)truncation_threshold)
               : v;
    }

    template <typename T>
    result_type operator()(const T &v) const {
      StateKeeper sk(const_cast<VarConvBase *>((const VarConvBase *)this));
      _ss << v;
      return _ss.str();
    }
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC VarCast : public boost::static_visitor<variant_t> {
  public:
    result_type operator()(const null_t &t, const std::string &v) {
      return t;
    }
    result_type operator()(const unknown_t &t, const std::string &v) {
      return v;
    }
    result_type operator()(const blob_ref_t &t, const std::string &v) {
      blob_ref_t res(new blob_t());
      res->reserve(v.size());
      std::copy(v.begin(), v.end(), std::back_inserter(*res));
      return res;
    }
    result_type operator()(const std::string &t, const std::string &v) {
      return v;
    }
    template <typename T>
    result_type operator()(const T &t, const std::string &v) {
      T res;
      std::stringstream ss(v);
      ss.precision(std::numeric_limits<long double>::digits10);
      ss >> res;
      return res;
    }
    template <typename T>
    result_type operator()(const T &t, const T &v) {
      return v;
    }
    result_type operator()(const null_t &t, const null_t &v) {
      return v;
    }
    template <typename T>
    result_type operator()(const T &t, const null_t &v) {
      return v;
    }
    template <typename T, typename V>
    result_type operator()(const T &t, const V &v) {
      return t;
      //! throw std::runtime_error("Conversion for types pair is not implemented");
    }
  };

  class VarToInt : public boost::static_visitor<std::int64_t> {
  public:
    result_type operator()(const int &v) const {
      return v;
    }
    result_type operator()(const std::int64_t &v) const {
      return v;
    }
    result_type operator()(const null_t &v) const {
      return 0;
    }

    template <typename T>
    result_type operator()(const T &v) const {
      return -1;
      //! throw std::runtime_error(std::string("Variant: wrong type: '")+typeid(T).name()+"' instead of
      //! '"+typeid(result_type).name()+"'");
    }
  };

  class VarToBool : public boost::static_visitor<bool> {
  public:
    result_type operator()(const bool &v) const {
      return v;
    }
    result_type operator()(const null_t &v) const {
      return false;
    }

    template <typename T>
    result_type operator()(const T &v) const {
      return false;
      //! throw std::runtime_error(std::string("Variant: wrong type: '")+typeid(T).name()+"' instead of
      //! '"+typeid(result_type).name()+"'");
    }
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC VarToLongDouble : public boost::static_visitor<long double> {
  public:
    result_type operator()(const long double &v) const {
      return v;
    }
    result_type operator()(const int &v) const {
      return v;
    }
    result_type operator()(const std::int64_t &v) const {
      return (long double)v;
    }
    result_type operator()(const null_t &v) const {
      return 0;
    }

    template <typename T>
    result_type operator()(const T &v) const {
      return -1;
      //! throw std::runtime_error(std::string("Variant: wrong type: '")+typeid(T).name()+"' instead of
      //! '"+typeid(result_type).name()+"'");
    }
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC QuoteVar : public VarConvBase {
  public:
    QuoteVar() : quote("'"), store_unknown_as_string(true), allow_func_escaping(false), bitMode(false), needQuote(true) {
    }
    typedef std::function<std::string(const std::string &)> Escape_sql_string;
    Escape_sql_string escape_string;
    std::string quote;
    typedef std::function<std::string(const unsigned char *, size_t)> Blob_to_string;
    Blob_to_string blob_to_string;
    bool store_unknown_as_string;
    bool allow_func_escaping;
    bool bitMode;
    bool needQuote;

    static std::string escape_ansi_sql_string(const std::string &text) // used by sqlite
    {
      std::string escaped;
      std::string::size_type p, s, len = text.length();

      // escapes '

      p = 0;
      s = 0;
      while (p < len) {
        switch (text[p]) {
          case '\'':
            if (p > s)
              escaped.append(text.substr(s, p - s));
            escaped.append("\'");
            escaped.append(text.substr(p, 1));
            s = p + 1;
            break;
        }
        p++;
      }
      if (p > s)
        escaped.append(text.substr(s));

      return escaped;
    }

    static std::string blob_to_hex_string(const unsigned char *data, size_t size) {
      static const char hex_dig[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
      std::string out(size * 2 + 2, ' ');
      std::string::iterator p = out.begin();
      *p++ = '0';
      *p++ = 'x';
      for (const unsigned char *d = data, *end = d + size; d < end; ++d) {
        *p++ = hex_dig[*d >> 4];
        *p++ = hex_dig[*d & 0x0F];
      }
      return out;
    }

    result_type operator()(const unknown_t &, const std::string &v) const {
      static std::string t;
      return store_unknown_as_string ? operator()(t, v) : v;
    }
    template <typename T>
    result_type operator()(const T &, const unknown_t &) const {
      return "";
    }
    template <typename T>
    result_type operator()(const T &, const null_t &) const {
      return "NULL";
    }
    template <typename T>
    result_type operator()(const T &x, const std::string &v) const {
      if (allow_func_escaping) {
        //! temporary convention to allow function call as a cell value
        // escape sequnce "\func " at start denotes the rest of the value is actual value and it should go unquoted
        // to enter "\func " value as literal user will have to enter "\\func "
        static const std::string func_call_seq = "\\func ";
        static const std::string func_call_exc = "\\\\func ";
        if (!v.empty() && (v[0] == '\\')) {
          if ((v.size() > func_call_seq.size()) && (v.compare(0, func_call_seq.size(), func_call_seq) == 0))
            return v.substr(func_call_seq.size());
          else if ((v.size() > func_call_exc.size()) && (v.compare(0, func_call_exc.size(), func_call_exc) == 0))
            return (needQuote ? ((bitMode ? "b" : "" ) + quote) : "") + escape_string(v.substr(1)) + (needQuote ? quote : "");
        }
      }
      return (needQuote ? ((bitMode ? "b" : "" ) + quote) : "") + escape_string(v) + (needQuote ? quote : "");
    }
    template <typename T>
    result_type operator()(const T &, const blob_ref_t &v) const {
      return !blob_to_string ? "?" /*bind variable placeholder*/ : blob_to_string(&(*v)[0], v->size());
    }
    result_type operator()(const blob_ref_t &, const blob_ref_t &v) const {
      return !blob_to_string ? "?" /*bind variable placeholder*/ : blob_to_string(&(*v)[0], v->size());
    }
    result_type operator()(const blob_ref_t &, const std::string &v) const {
      return !blob_to_string ? "?" /*bind variable placeholder*/ : blob_to_string((const unsigned char *)v.data(),
                                                                                  v.size());
    }
    result_type operator()(const blob_ref_t &, const null_t &) const {
      return !blob_to_string ? "?" /*bind variable placeholder*/ : "NULL";
    }

    template <typename T, typename V>
    result_type operator()(const T &, const V &v) const {
      StateKeeper sk(const_cast<VarConvBase *>((const VarConvBase *)this));
      _ss << v;
      return _ss.str();
    }
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC TypeOfVar : public boost::static_visitor<std::string> {
  public:
    result_type operator()(const long double &v) const {
      return "FLOAT";
    }
    result_type operator()(const int &v) const {
      return "INTEGER";
    }
    result_type operator()(const std::string &v) const {
      return "VARCHAR";
    }
    result_type operator()(const blob_ref_t &v) const {
      return "BLOB";
    }

    template <typename T>
    result_type operator()(const T &v) const {
      return "VARCHAR";
    }
  };

  WBPUBLICBACKEND_PUBLIC_FUNC bool is_var_null(const sqlite::variant_t &value);
  WBPUBLICBACKEND_PUBLIC_FUNC bool is_var_unknown(const sqlite::variant_t &value);
  WBPUBLICBACKEND_PUBLIC_FUNC bool is_var_blob(const sqlite::variant_t &value);

  WBPUBLICBACKEND_PUBLIC_FUNC void optimize_sqlite_connection_for_speed(sqlite::connection *conn);

  class WBPUBLICBACKEND_PUBLIC_FUNC Sqlite_transaction_guarder {
  public:
    Sqlite_transaction_guarder(sqlite::connection *conn, bool use_immediate = true);
    ~Sqlite_transaction_guarder();
    void commit();
    void commit_and_start_new_transaction();

  private:
    sqlite::connection *_conn;
    bool _in_trans;
  };
}

typedef size_t RowId;
typedef size_t ColumnId;
typedef std::vector<sqlite::variant_t> Data;

template <typename C>
inline void reinit(C &c) {
  C tmp;
  c.swap(tmp);
}

template <typename T>
class AutoSwap {
public:
  AutoSwap(T &v1, T &v2) : _v1(v1), _v2(v2) {
    std::swap(_v1, _v2);
  }
  ~AutoSwap() {
    std::swap(_v1, _v2);
  }

private:
  T &_v1;
  T &_v2;
};

// double WBPUBLICBACKEND_PUBLIC_FUNC timestamp();
std::tm WBPUBLICBACKEND_PUBLIC_FUNC local_timestamp();
std::string WBPUBLICBACKEND_PUBLIC_FUNC format_time(const std::tm &t, const char *format = "%H:%M:%S");
std::string WBPUBLICBACKEND_PUBLIC_FUNC current_time(const char *format = "%H:%M:%S");

#endif /* _SQLIDE_GENERICS_H_ */
