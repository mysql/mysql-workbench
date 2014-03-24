using System;
using System.Windows.Forms;
using MySQL.Workbench;

namespace MySQL.GUI.Workbench
{
  public partial class ExceptionDialog : Form
  {
    private static ExceptionDialog singleton = new ExceptionDialog();

    private String errorInfo;

    private WbContext wbContext;

    protected ExceptionDialog()
    {
      InitializeComponent();
    }

    public static void Show(String message, String info, WbContext wbcontext)
    {
      singleton.messageLabel.Text = message;
      singleton.errorInfo = info;
      singleton.wbContext = wbcontext;
      singleton.ShowDialog();
    }

    private void reportBugButton_Click(object sender, EventArgs e)
    {
      if (wbContext.is_commercial())
        System.Diagnostics.Process.Start("http://support.oracle.com");
      else
        //wbContext.report_bug(errorInfo);
        System.Diagnostics.Process.Start("http://bugs.mysql.com");
    }

    private void copyInfoButton_Click(object sender, EventArgs e)
    {
      Clipboard.SetText(errorInfo);
    }

    private void copyStackTraceToClipboardToolStripMenuItem_Click(object sender, EventArgs e)
    {
      Clipboard.SetText(errorInfo);
    }

  }
}
