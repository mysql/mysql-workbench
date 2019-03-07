/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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
using System.Collections.Generic;
using System.Drawing.Printing;
using System.Windows.Forms;

using MySQL.Grt;

namespace MySQL.GUI.Workbench.Plugins
{
  public class PrintDialog : Plugin
  {
    System.Windows.Forms.PrintDialog printDialog;
    PrintDocument printDocument;
    GrtValue grtArguments;
    int pageNumber;
    int pageCount;


    public PrintDialog(GrtManager GrtManager, GrtValue GrtList)
      : base(GrtManager, GrtList)
		{
      grtArguments = GrtList;
    }

    public override void Execute()
    {
      printDocument = new PrintDocument();

      printDocument.OriginAtMargins = true;
      printDocument.BeginPrint += new PrintEventHandler(printDocument_BeginPrint);
      printDocument.PrintPage += new PrintPageEventHandler(printDocument_PrintPage);

      Dictionary<String, Object> paperSettings = Printing.getPaperSettings(grtArguments);
      printDocument.DefaultPageSettings.Landscape = (string)paperSettings["orientation"] == "landscape";

      // Sizes must be given in inch * 100 (sigh).
      int paperWidth = (int)Math.Round((double)paperSettings["width"] / 0.254);
      int paperHeight = (int)Math.Round((double)paperSettings["height"] / 0.254);
      PaperSize paperSize = new PaperSize("Doesn't matter", paperWidth, paperHeight);
      printDocument.DefaultPageSettings.PaperSize = paperSize;

      if ((bool)paperSettings["marginsSet"])
        printDocument.DefaultPageSettings.Margins =
          new Margins((int)paperSettings["marginLeft"], (int)paperSettings["marginRight"],
            (int)paperSettings["marginTop"], (int)paperSettings["marginBottom"]);

      printDialog = new System.Windows.Forms.PrintDialog();
      printDialog.Document = printDocument;
      printDialog.AllowPrintToFile = true;

      pageNumber = 0;
      pageCount = -1;

      if (printDialog.ShowDialog() == DialogResult.OK)
      {
        printDocument.Print();
      }
    }

    void printDocument_BeginPrint(object sender, PrintEventArgs e)
    {
      pageNumber = 0;
      pageCount = -1;
    }

    void printDocument_PrintPage(object sender, PrintPageEventArgs e)
    {
      if (pageCount < 0)
        pageCount = Printing.getPageCount(grtArguments);

      if (Printing.printPageHDC(grtArguments, pageNumber++, e.Graphics.GetHdc(),
        e.PageSettings.PaperSize.Width, e.PageSettings.PaperSize.Height) < 1)
        e.HasMorePages = false;
      else
        e.HasMorePages = pageNumber < pageCount;
    }

  }
}
