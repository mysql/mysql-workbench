/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;

namespace mga.net {
  class Program {

    #region Static Variables and Enums

    /// <summary>
    /// Application context.
    /// </summary>
    private static ApplicationContextWr _controller = new ApplicationContextWr();

    #endregion

    [STAThread]
    static int Main(string[] args) {
      int ret = 0;
      try {
        var env = Environment.GetEnvironmentVariables();
        IList envNames = new List<String>();
        foreach (DictionaryEntry item in env) {
          envNames.Add(item.Key + "=" + item.Value);
        }
        _controller = new ApplicationContextWr();
        if (_controller != null) {
          if ((ret = _controller.initialize()) != 0)
            return ret;
          var asm = Assembly.GetEntryAssembly();
          if ((ret = _controller.parseParams(args, Path.GetDirectoryName(asm.Location), envNames.Cast<String>().ToArray())) != 0)
            return ret;
          _controller.run();
          ret = _controller.shutDown();
        }
      } catch (Exception ex) {
        Console.Write(ex.Message);
        ret = 1;
      } finally {
        _controller.Dispose();
      }
      return ret;
    }
  }
}
