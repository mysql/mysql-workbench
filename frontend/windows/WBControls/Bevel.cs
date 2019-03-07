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

using System.ComponentModel;
using System.Drawing;
using System.Windows.Forms;

namespace MySQL.Utilities
{
	public enum BevelStyleType { Lowered, Raised, Flat, White, Dark }

	public partial class Bevel : System.Windows.Forms.Panel
	{
		/// <summary> 
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		private BevelStyleType bevelStyle = BevelStyleType.Flat;
		private Border3DSide borderSide = Border3DSide.Top;

		public Bevel()
		{
			InitializeComponent();

			// Set double buffer
			SetStyle(
						ControlStyles.UserPaint |
						ControlStyles.AllPaintingInWmPaint |
						ControlStyles.OptimizedDoubleBuffer, true);
		}

		/// <summary> 
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose(bool disposing)
		{
			if (disposing && (components != null))
			{
				components.Dispose();
			}
			base.Dispose(disposing);
		}

		#region Component Designer generated code

		/// <summary> 
		/// Required method for Designer support - do not modify 
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			components = new System.ComponentModel.Container();
		}

		#endregion

		protected override void OnPaint(PaintEventArgs e)
		{
			Graphics g = e.Graphics;
			Rectangle r = ClientRectangle;

			if (BevelStyle == BevelStyleType.White || BevelStyle == BevelStyleType.Dark)
			{
				Pen p;

				if (BevelStyle == BevelStyleType.White)
					p = new Pen(Color.White);
				else
					p = new Pen(SystemColors.ControlDark);

				switch (borderSide)
				{
					case Border3DSide.Left:
						g.DrawLine(p, r.Left, r.Top, r.Left, r.Bottom - 1);
						break;
					case Border3DSide.Right:
						g.DrawLine(p, r.Right, r.Top, r.Right - 1, r.Bottom - 1);
						break;
					case Border3DSide.Top:
						g.DrawLine(p, r.Left, r.Top, r.Right - 1, r.Top);
						break;
          case Border3DSide.Bottom:
            g.DrawLine(p, r.Left, r.Bottom - 1, r.Right, r.Bottom - 1);
            break;
          case Border3DSide.Middle:
						g.DrawLine(p, r.Left, (r.Top + r.Bottom) / 2, r.Right, (r.Top + r.Bottom) / 2);
						break;
          default: // All sides.
            g.DrawRectangle(p, r);
            break;
        }

				p.Dispose();
			}
			else if (BevelStyle != BevelStyleType.Flat)
			{
				Border3DStyle s = Border3DStyle.SunkenOuter;
				if (BevelStyle == BevelStyleType.Raised)
					s = Border3DStyle.RaisedInner;

				switch (borderSide)
				{
					case Border3DSide.All:
						ControlPaint.DrawBorder3D(g, r, s);
						break;
					case Border3DSide.Left:
						ControlPaint.DrawBorder3D(g, r.Left, r.Top, 2, r.Top, s);
						break;
					case Border3DSide.Right:
						ControlPaint.DrawBorder3D(g, r.Right - 2, r.Top, 2, r.Height, s);
						break;
					case Border3DSide.Top:
						ControlPaint.DrawBorder3D(g, r.Left, r.Top, r.Width, 2, s);
						break;
					default:
						ControlPaint.DrawBorder3D(g, r.Left, r.Bottom - 2, r.Width, 2, s);
						break;
				}
			}

			base.OnPaint(e);
		}

		#region Properties

		[Bindable(true), Category("Appearance"),
		Description("The style the bevel should use.")]
		public BevelStyleType BevelStyle
		{
			get { return bevelStyle; }
			set
			{
				bevelStyle = value;
				Invalidate();
			}
		}

		[Bindable(true), Category("Appearance"),
		Description("The style the bevel should use.")]
		public Border3DSide BorderSide
		{
			get { return borderSide; }
			set
			{
				borderSide = value;
				Invalidate();
			}
		}

		#endregion
	}
}
