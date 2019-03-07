/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace MySQL.Utilities
{
  public class StringUtilities
  {
    /**
     * Adjusts the given string so that it fits into the given width. EllipsisWidth gives the width of
     * the three points to be added to the shorted string. If this value is 0 then it will be determined implicitely.
     * For higher speed (and multiple entries to be shorted) specify this value explicitely.
     */
    public static String ShortenString(String text, int targetWidth, Font font)
    {
      String result;
      int length = text.Length;
      if (length == 0 || targetWidth <= 0)
        result = "";
      else
      {
        // Proposed size is set to the target width as maximum.
        Size proposedSize = new Size(targetWidth, int.MaxValue);
        Size size = TextRenderer.MeasureText(text, font, proposedSize, TextFormatFlags.EndEllipsis | 
          TextFormatFlags.ModifyString);

        // MeasureText insert ellipsis and a null terminator, so we only return chars up to that terminator.
        String[] parts = text.Split('\0');
        if (parts.Length < 1)
          result = text;
        else
          result = parts[0];
      };
      return result;
    }
  }
}
