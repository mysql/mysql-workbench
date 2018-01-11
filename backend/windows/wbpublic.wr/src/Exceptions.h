/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __EXCEPTIONS_H__
#define __EXCEPTIONS_H__

#ifdef _MSC_VER
using namespace System;
using namespace System::Collections::Generic;
#endif

#include <sstream>

namespace MySQL {
  namespace Grt {

  public
    ref class BackendException : public System::Exception {
      //  std::exception *inner;
      String ^ _message;

    public:
      BackendException(std::exception *inn) : _message(CppStringToNative(inn->what())) {
      }

      BackendException(const std::exception &inn) : _message(CppStringToNative(inn.what())) {
      }

      BackendException(const char *message) : _message(CppStringToNative(message)) {
      }

      BackendException(const std::string &message) : _message(CppStringToNative(message)) {
      }

      virtual property String ^ Message {
        String ^ get() override {
          return _message;
        }
      }
    };

  public
    ref class UnknownBackendException : public BackendException {
      std::string itoa(int i) {
        std::stringstream ss;
        ss << i;
        std::string res = ss.str().c_str();
        return res;
      }

    public:
      UnknownBackendException(const char *file, int line)
        : BackendException("Unknown Exception caught in " + std::string(file) + " at line " + itoa(line)) {
      }
    };

  } // namespace Grt
} // namespace MySQL

#endif // __EXCEPTIONS_H__