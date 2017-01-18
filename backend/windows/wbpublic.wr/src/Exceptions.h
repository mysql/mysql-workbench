/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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