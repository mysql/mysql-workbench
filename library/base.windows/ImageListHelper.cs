/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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
using System.Text;
using System.Windows.Forms;
using System.Drawing;
using System.Drawing.Imaging;
using System.Reflection;
using System.Runtime.InteropServices;

namespace MySQL.Utilities
{
	public class ImageListHelper
	{
		[StructLayout(LayoutKind.Sequential)]
		private class BitmapInfo
		{
			public Int32 biSize;
			public Int32 biWidth;
			public Int32 biHeight;
			public Int16 biPlanes;
			public Int16 biBitCount;
			public Int32 biCompression;
			public Int32 biSizeImage;
			public Int32 biXPelsPerMeter;
			public Int32 biYPelsPerMeter;
			public Int32 biClrUsed;
			public Int32 biClrImportant;
			public Int32 bmiColors;
		};

		[DllImport("comctl32.dll")]
		private static extern bool ImageList_Add(IntPtr hImageList, IntPtr hBitmap, IntPtr hMask);
		[DllImport("kernel32.dll")]
		private static extern bool RtlMoveMemory(IntPtr dest, IntPtr source, int dwcount);
		[DllImport("gdi32.dll")]
		private static extern IntPtr CreateDIBSection(IntPtr hdc, [In, MarshalAs(UnmanagedType.LPStruct)]BitmapInfo pbmi, uint iUsage, out IntPtr ppvBits, IntPtr hSection, uint dwOffset);

		private static Type imageInfoType;
		private static FieldInfo imageFieldInfo;

		static ImageListHelper() 
		{
			imageInfoType = typeof(ImageList).Assembly.GetType("System.Windows.Forms.ImageList+ImageCollection+ImageInfo");
			imageFieldInfo = typeof(ImageList.ImageCollection).GetField("imageInfoCollection", BindingFlags.NonPublic | BindingFlags.Instance);
		}

		public static void Add(string FileName, ImageList Imagelist)
		{
			Bitmap bitmap = new Bitmap(FileName);

			if (bitmap.RawFormat.Guid == ImageFormat.Icon.Guid)
			{
				Icon icon = new Icon(FileName);

				Imagelist.Images.Add(Icon.FromHandle(icon.Handle));
				icon.Dispose();
			}
			else
				Add(bitmap, Imagelist);

			bitmap.Dispose();
		}

		public static void RestoreImageInfo(ImageList Imagelist)
		{
			if (Imagelist != null)
			{
				ArrayList imageInfoCollection = 
					(ArrayList)imageFieldInfo.GetValue(Imagelist.Images);

				imageInfoCollection.Add(Activator.CreateInstance(imageInfoType));
			}
		}

		public static int Add(Bitmap Bitmap, ImageList ImageList)
		{
			IntPtr hBitmap;
			IntPtr ppvBits;
			BitmapInfo bitmapInfo = new BitmapInfo();

      // Make a copy of the bitmap. We are are going to flip it and don't want the
      // original file to be modified. This solves another problem en-passant
      // where bitmap and image list size differ.
      Bitmap = new Bitmap(Bitmap, ImageList.ImageSize.Width, ImageList.ImageSize.Height);

			bitmapInfo.biSize = 40;
			bitmapInfo.biBitCount = 32;
			bitmapInfo.biPlanes = 1;
			bitmapInfo.biWidth = Bitmap.Width;
			bitmapInfo.biHeight = Bitmap.Height;

			Bitmap.RotateFlip(RotateFlipType.RotateNoneFlipY);
			hBitmap = CreateDIBSection(new IntPtr(0), bitmapInfo, 0, 
				out ppvBits, new IntPtr(0), 0);

			BitmapData bitmapData = Bitmap.LockBits(
				new Rectangle(0, 0, Bitmap.Width, Bitmap.Height), 
				ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
			RtlMoveMemory(ppvBits, bitmapData.Scan0, Bitmap.Height * bitmapData.Stride);

			Bitmap.UnlockBits(bitmapData);

			ImageList_Add(ImageList.Handle, hBitmap, new IntPtr(0));

			return ImageList.Images.Count - 1;
		}
	}

}
