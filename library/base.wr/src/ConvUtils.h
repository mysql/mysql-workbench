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

#pragma once

namespace MySQL {

  // Note: don't waste your time trying to prettify this code or even make it into a static class.
  //       You won't be able to use any of the functions outside this assembly.
  //       The reason is the native types which are since VS 2005 private by default
  //       and you cannot use pragma make_public() for templated types (big sigh!).
  static System::String ^
    CppStringToNative(const std::string &str) {
      if (str.length() == 0)
        return "";

      if (str.find('\r') == std::string::npos) {
        // convert from \n to \r\n (but don't double-convert \r\n to \r\r\n)
        System::String ^ result = gcnew System::String(str.c_str(), 0, (int)str.size(), System::Text::Encoding::UTF8);
        return result->Replace("\n", "\r\n");
      }
      return gcnew System::String(str.c_str(), 0, (int)str.size(), System::Text::Encoding::UTF8);
    }

    static System::String
    ^
    CppStringToNativeRaw(const std::string &str) {
      if (str.length() == 0)
        return "";

      return gcnew System::String(str.c_str(), 0, (int)str.size(), System::Text::Encoding::UTF8);
    }

    static std::string NativeToCppString(System::String ^ str) {
    if (str == nullptr || str->Length == 0)
      return "";

    array<unsigned char> ^ chars = System::Text::Encoding::UTF8->GetBytes(str->Replace("\r\n", "\n"));
    if (chars == nullptr || chars->Length == 0)
      return "";

    pin_ptr<unsigned char> char_ptr = &chars[0];
    std::string result((char *)char_ptr);
    return result;
  }

  static std::string NativeToCppStringRaw(System::String ^ str) {
    if (str == nullptr || str->Length == 0)
      return "";

    array<unsigned char> ^ chars = System::Text::Encoding::UTF8->GetBytes(str);
    if (chars == nullptr || chars->Length == 0)
      return "";

    pin_ptr<unsigned char> char_ptr = &chars[0];
    std::string result((char *)char_ptr);
    return result;
  }

  static System::Collections::Generic::List<System::String ^> ^
    CppStringListToNative(const std::vector<std::string> &input) {
      int cap = static_cast<int>(input.size());
      System::Collections::Generic::List<System::String ^> ^ result =
        gcnew System::Collections::Generic::List<System::String ^>(cap);
      std::vector<std::string>::const_iterator e = input.end();

      for (std::vector<std::string>::const_iterator i = input.begin(); i != e; i++)
        result->Add(CppStringToNative(*i));
      return result;
    }

    static std::vector<std::string> NativeToCppStringList(System::Collections::Generic::List<System::String ^> ^
                                                          input) {
    std::vector<std::string> result;
    result.reserve(input->Count);
    for each(System::String ^ str_item in input) result.push_back(NativeToCppString(str_item));
    return result;
  }

  static std::list<std::string> NativeToCppStringList2(System::Collections::Generic::List<System::String ^> ^ input) {
    std::list<std::string> result;
    for each(System::String ^ str_item in input) result.push_back(NativeToCppString(str_item));
    return result;
  }

  static System::Collections::Generic::List<System::String ^> ^
    CppStringListToNative2(const std::list<std::string> &input) {
      int cap = static_cast<int>(input.size());
      System::Collections::Generic::List<System::String ^> ^ result =
        gcnew System::Collections::Generic::List<System::String ^>(cap);
      std::list<std::string>::const_iterator e = input.end();

      for (std::list<std::string>::const_iterator i = input.begin(); i != e; i++)
        result->Add(CppStringToNative(*i));
      return result;
    }

    static System::Collections::Generic::List<int> ^
    CppVectorToIntList(const std::vector<int> &input) {
      typedef const std::vector<int> SourceContainerType;
      typedef System::Collections::Generic::List<int> TargetContainerType;

      TargetContainerType ^ result = gcnew TargetContainerType(static_cast<int>(input.size()));
      SourceContainerType::const_iterator e = input.end();

      for (SourceContainerType::const_iterator i = input.begin(); i != e; i++)
        result->Add(*i);

      return result;
    }

    static std::vector<int> IntListToCppVector(System::Collections::Generic::List<int> ^ input) {
    typedef const System::Collections::Generic::List<int> ^ SourceContainerType;
    typedef std::vector<int> TargetContainerType;

    TargetContainerType result;
    result.reserve(input->Count);

    for each(int listitem in input) result.push_back(listitem);

    return result;
  }

  static std::vector<size_t> IntListToCppVector2(System::Collections::Generic::List<int> ^ input) {
    typedef const System::Collections::Generic::List<int> ^ SourceContainerType;
    typedef std::vector<size_t> TargetContainerType;

    TargetContainerType result;
    result.reserve(input->Count);

    for each(int listitem in input) result.push_back(listitem);

    return result;
  }

  static System::Collections::Generic::Dictionary<System::String ^, System::String ^> ^
    CppStringMapToDictionary(const std::map<std::string, std::string> &input) {
      typedef const std::map<std::string, std::string> SourceContainerType;
      typedef System::Collections::Generic::Dictionary<System::String ^, System::String ^> TargetContainerType;

      TargetContainerType ^ result = gcnew TargetContainerType(static_cast<int>(input.size()));
      SourceContainerType::const_iterator e = input.end();

      for (SourceContainerType::const_iterator iterator = input.begin(); iterator != input.end(); ++iterator)
        result->Add(CppStringToNativeRaw(iterator->first), CppStringToNativeRaw(iterator->second));

      return result;
    }

} // namespace MySQL

// C++/CLI variant of the C# is operator.
template <typename T, class O>
System::Boolean is(O o) {
  return dynamic_cast<T ^>(o) != nullptr;
}

/**
 * A more general conversion routine from any managed collection to a std::vector.
 * For strings no new line conversion is done. The used encoding is that of the system.
 */
template <typename T>
generic<typename S> std::vector<T> marshal_as(System::Collections::Generic::ICollection<S> ^ list) {
  if (list == nullptr)
    throw gcnew System::ArgumentNullException(L"list");

  std::vector<T> result;
  result.reserve(list->Count);
  for
    each(S & element in list) result.push_back(marshal_as<T>(element));

  return result;
}
