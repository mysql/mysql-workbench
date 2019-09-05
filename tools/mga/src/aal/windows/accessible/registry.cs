
/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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


using Microsoft.Win32;
using System;
using System.Linq;

namespace aal {
  public sealed class RegistryNet {
    private static readonly Lazy<RegistryNet> lazy =
       new Lazy<RegistryNet>(() => new RegistryNet());

    // -----------------------------------------------------------------------------------------------------------------

    public static RegistryNet Instance => lazy.Value;

    // -----------------------------------------------------------------------------------------------------------------

    private RegistryNet() {
    }

    // -----------------------------------------------------------------------------------------------------------------

    public void SetValue(RegistryHive rkey,
                  string keyName,
                  string valueName,
                  object value,
                  RegistryValueKind type,
                  RegistryView view) {
      using(var hkey = RegistryKey.OpenBaseKey(rkey, view)) {
        using(var subKey = hkey.OpenSubKey(keyName)) {
          var names = subKey.GetValueNames();
          if (names != null && names.Contains(valueName, StringComparer.CurrentCultureIgnoreCase)) {
            if (subKey.GetValueKind(valueName) == type) {
              subKey.SetValue(keyName, value);
            }
          }
        }
      }
    }

    // -----------------------------------------------------------------------------------------------------------------

    public object GetValue(RegistryHive rkey,
                    string keyName,
                    string valueName,
                    RegistryView view) {
      using(var hkey = RegistryKey.OpenBaseKey(rkey, view)) {
        using(var subKey = hkey.OpenSubKey(keyName)) {
          var names = subKey.GetValueNames();
          if (names != null && names.Contains(valueName, StringComparer.CurrentCultureIgnoreCase)) {
            return subKey.GetValue(valueName);
          }
        }
      }
      return null;
    }

    // -----------------------------------------------------------------------------------------------------------------

    public bool CreateKey(RegistryHive rkey,
                  string keyName,
                  string valueName,
                  object value,
                  RegistryValueKind type,
                  RegistryView view) {
      using(var hkey = RegistryKey.OpenBaseKey(rkey, view)) {
        using(var subKey = hkey.OpenSubKey(keyName)) {
          if (subKey != null) {
            subKey.SetValue(valueName, value, type);
            return true;
          } else {
            var newKey = hkey.CreateSubKey(keyName);
            newKey.SetValue(valueName, value, type);
            newKey.Close();
            return true;
          }
        }
      }
    }

    // -----------------------------------------------------------------------------------------------------------------

    public bool DeleteKey(RegistryHive rkey,
                  string keyName,
                  bool deleteSubTree,
                  RegistryView view) {
      using(var hkey = RegistryKey.OpenBaseKey(rkey, view)) {
        using(var subKey = hkey.OpenSubKey(keyName)) {
          if (subKey != null) {
            if (deleteSubTree) {
              hkey.DeleteSubKey(keyName);
            } else {
              hkey.DeleteSubKey(keyName);
            }
            return true;
          }
        }
        return false;
      }
    }

    // -----------------------------------------------------------------------------------------------------------------

    public bool HasKey(RegistryHive rkey,
                  string keyName,
                  RegistryView view) {
      using(var hkey = RegistryKey.OpenBaseKey(rkey, view)) {
        using(var subKey = hkey.OpenSubKey(keyName)) {
          return subKey != null ? true : false;
        }
      }
    }

    // -----------------------------------------------------------------------------------------------------------------

    public bool HasValue(RegistryHive rkey,
                  string keyName,
                  string valueName,
                  bool checkValueType,
                  RegistryValueKind type,
                  RegistryView view) {
      using(var hkey = RegistryKey.OpenBaseKey(rkey, view)) {
        using(var subKey = hkey.OpenSubKey(keyName)) {
          var names = subKey.GetValueNames();
          if (names != null && names.Contains(valueName, StringComparer.CurrentCultureIgnoreCase)) {
            if (checkValueType) {
              return subKey.GetValueKind(valueName) == type;
            } else {
              return true;
            }
          }
        }
      }
      return false;
    }

    // -----------------------------------------------------------------------------------------------------------------

    public bool DeleteValue(RegistryHive rkey,
                  string keyName,
                  string valueName,
                  RegistryView view) {
      throw new NotImplementedException();
    }

    // -----------------------------------------------------------------------------------------------------------------
  }
}
