/* 
 * Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
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

using System;
using System.Collections.Generic;
using System.Drawing.Printing;
using System.Windows.Forms;

using MySQL.Grt;

namespace MySQL.GUI.Workbench.Plugins
{
  class PrintPreviewDialog : Plugin
  {
    PrintDocument printDocument;
    GrtValue grtArguments;
    int pageNumber;
    int pageCount;


    public PrintPreviewDialog(GrtManager GrtManager, GrtValue GrtList)
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
      PaperSize paperSize = new PaperSize("Ignored anyway", paperWidth, paperHeight);

      if ((bool)paperSettings["marginsSet"])
        printDocument.DefaultPageSettings.Margins =
          new Margins((int)paperSettings["marginLeft"], (int)paperSettings["marginRight"],
            (int)paperSettings["marginTop"], (int)paperSettings["marginBottom"]);

      System.Windows.Forms.PrintPreviewDialog preview = new System.Windows.Forms.PrintPreviewDialog();

      preview.Document = printDocument;
      preview.UseAntiAlias = true;

      preview.ShowDialog();
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

      e.Graphics.DrawRectangle(new System.Drawing.Pen(System.Drawing.Color.Red), 0, 0,
        e.PageSettings.PaperSize.Width - 1, e.PageSettings.PaperSize.Height - 1);

      e.Graphics.DrawRectangle(new System.Drawing.Pen(System.Drawing.Color.Green), e.PageBounds);

      IntPtr hdc = e.Graphics.GetHdc();

      if (Printing.printPageHDC(grtArguments, pageNumber++, hdc,
        e.PageSettings.PaperSize.Width, e.PageSettings.PaperSize.Height) < 1)
        e.HasMorePages = false;
      else
        e.HasMorePages = pageNumber < pageCount;
      e.Graphics.ReleaseHdc();
    }

  }
}
