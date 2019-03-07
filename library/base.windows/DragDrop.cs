/*
 * Copyright (c) 2013, 2019, Oracle and/or its affiliates. All rights reserved.
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
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;

using MySQL.Utilities.SysUtils;
using System.IO;
using System.Runtime.Serialization.Formatters.Binary;
using System.ComponentModel;

namespace MySQL.Utilities
{
  #region IDataObject extensions

  public static class WBIDataObjectExtensions
  {
    public static void SetDataEx(this IDataObject dataObject, string format, object data)
    {
      System.Windows.Forms.DataFormats.Format dataFormat = System.Windows.Forms.DataFormats.GetFormat(format);

      // Initialize the format structure
      FORMATETC formatETC = new FORMATETC();
      formatETC.cfFormat = (short)dataFormat.Id;
      formatETC.dwAspect = DVASPECT.DVASPECT_CONTENT;
      formatETC.lindex = -1;
      formatETC.ptd = IntPtr.Zero;

      // Try to discover the TYMED from the format and data
      TYMED tymed = GetCompatibleTymed(format, data);

      // If a TYMED was found, we can use the system DataObject
      // to convert our value for us.
      if (tymed != TYMED.TYMED_NULL)
      {
        formatETC.tymed = tymed;

        // Use a temporary standard data object to convert from managed to native.
        System.Windows.Forms.DataObject conv = new System.Windows.Forms.DataObject();
        conv.SetData(format, true, data);

        STGMEDIUM medium;
        ((IDataObject)conv).GetData(ref formatETC, out medium);
        try
        {
          ((IDataObject)dataObject).SetData(ref formatETC, ref medium, true);
        }
        catch
        {
          Win32.ReleaseStgMedium(ref medium);
          throw;
        }
      }
      else
      {
        SetManagedData((IDataObject)dataObject, format, data);
      }
    }

    public static object GetManagedData(this IDataObject dataObject, string format)
    {
      FORMATETC formatETC;
      FillFormatETC(format, TYMED.TYMED_HGLOBAL, out formatETC);

      STGMEDIUM medium;
      dataObject.GetData(ref formatETC, out medium);

      IStream nativeStream;
      try
      {
        int hr = Win32.CreateStreamOnHGlobal(medium.unionmember, true, out nativeStream);
        if (hr != 0)
          return null;
      }
      finally
      {
        Win32.ReleaseStgMedium(ref medium);
      }

      // Convert the native stream to a managed stream.
      System.Runtime.InteropServices.ComTypes.STATSTG statstg;
      nativeStream.Stat(out statstg, 0);
      if (statstg.cbSize > int.MaxValue)
        throw new NotSupportedException();
      byte[] buf = new byte[statstg.cbSize];
      nativeStream.Read(buf, (int)statstg.cbSize, IntPtr.Zero);
      MemoryStream dataStream = new MemoryStream(buf);

      // Check for our stamp
      int sizeOfGuid = Marshal.SizeOf(typeof(Guid));
      byte[] guidBytes = new byte[sizeOfGuid];
      if (dataStream.Length >= sizeOfGuid)
      {
        if (sizeOfGuid == dataStream.Read(guidBytes, 0, sizeOfGuid))
        {
          Guid guid = new Guid(guidBytes);
          if (ManagedDataStamp.Equals(guid))
          {
            // Stamp matched, so deserialize
            BinaryFormatter formatter = new BinaryFormatter();
            Type dataType = (Type)formatter.Deserialize(dataStream);
            object data2 = formatter.Deserialize(dataStream);
            if (data2.GetType() == dataType)
              return data2;
            else if (data2 is string)
              return ConvertDataFromString((string)data2, dataType);
            else
              return null;
          }
        }
      }

      // Stamp didn't match... attempt to reset the seek pointer
      if (dataStream.CanSeek)
        dataStream.Position = 0;
      return null;
    }

    public static void SetManagedData(this IDataObject dataObject, string format, object data)
    {
      FORMATETC formatETC;
      FillFormatETC(format, TYMED.TYMED_HGLOBAL, out formatETC);

      // Serialize/marshal our data into an unmanaged medium.
      STGMEDIUM medium;
      GetMediumFromObject(data, out medium);
      try
      {
        dataObject.SetData(ref formatETC, ref medium, true);
      }
      catch
      {
        Win32.ReleaseStgMedium(ref medium);
        throw;
      }
    }

    private static bool IsFormatEqual(string formatA, string formatB)
    {
      return string.CompareOrdinal(formatA, formatB) == 0;
    }

    private static TYMED GetCompatibleTymed(string format, object data)
    {
      if (IsFormatEqual(format, System.Windows.Forms.DataFormats.Bitmap) && data is System.Drawing.Bitmap)
        return TYMED.TYMED_GDI;

      if (IsFormatEqual(format, System.Windows.Forms.DataFormats.EnhancedMetafile))
        return TYMED.TYMED_ENHMF;

      if (data is Stream
          || IsFormatEqual(format, System.Windows.Forms.DataFormats.Html)
          || IsFormatEqual(format, System.Windows.Forms.DataFormats.Text) || IsFormatEqual(format, System.Windows.Forms.DataFormats.Rtf)
          || IsFormatEqual(format, System.Windows.Forms.DataFormats.OemText)
          || IsFormatEqual(format, System.Windows.Forms.DataFormats.UnicodeText) || IsFormatEqual(format, "ApplicationTrust")
          || IsFormatEqual(format, System.Windows.Forms.DataFormats.FileDrop)
          || IsFormatEqual(format, "FileName")
          || IsFormatEqual(format, "FileNameW"))
        return TYMED.TYMED_HGLOBAL;

      if (IsFormatEqual(format, System.Windows.Forms.DataFormats.Dib) && data is System.Drawing.Image)
        return TYMED.TYMED_NULL;

      if (IsFormatEqual(format, typeof(System.Drawing.Bitmap).FullName))
        return TYMED.TYMED_HGLOBAL;

      if (IsFormatEqual(format, System.Windows.Forms.DataFormats.EnhancedMetafile) || data is System.Drawing.Imaging.Metafile)
        return TYMED.TYMED_NULL;

      if (IsFormatEqual(format, System.Windows.Forms.DataFormats.Serializable) || (data is System.Runtime.Serialization.ISerializable)
          || ((data != null) && data.GetType().IsSerializable))
        return TYMED.TYMED_HGLOBAL;

      return TYMED.TYMED_NULL;
    }

    private static void FillFormatETC(string format, TYMED tymed, out FORMATETC formatETC)
    {
      formatETC.cfFormat = (short)Win32.RegisterClipboardFormat(format);
      formatETC.dwAspect = DVASPECT.DVASPECT_CONTENT;
      formatETC.lindex = -1;
      formatETC.ptd = IntPtr.Zero;
      formatETC.tymed = tymed;
    }

    private static readonly Guid ManagedDataStamp = new Guid("D98D9FD6-FA46-4716-A769-F3451DFBE4B4");

    private static TypeConverter GetTypeConverterForType(Type dataType)
    {
      TypeConverterAttribute[] typeConverterAttrs = (TypeConverterAttribute[])dataType.GetCustomAttributes(typeof(TypeConverterAttribute), true);
      if (typeConverterAttrs.Length > 0)
      {
        Type convType = Type.GetType(typeConverterAttrs[0].ConverterTypeName);
        return (TypeConverter)Activator.CreateInstance(convType);
      }

      return null;
    }

    private static object ConvertDataFromString(string data, Type dataType)
    {
      TypeConverter conv = GetTypeConverterForType(dataType);
      if (conv != null && conv.CanConvertFrom(typeof(string)))
        return conv.ConvertFromInvariantString(data);

      throw new NotSupportedException("Cannot convert data");
    }

    private static object GetAsSerializable(object obj)
    {
      if (obj.GetType().IsSerializable)
        return obj;

      // Attempt type conversion to a string, but only if we know it can be converted back
      TypeConverter conv = GetTypeConverterForType(obj.GetType());
      if (conv != null && conv.CanConvertTo(typeof(string)) && conv.CanConvertFrom(typeof(string)))
        return conv.ConvertToInvariantString(obj);

      throw new NotSupportedException("Cannot serialize the object");
    }

    private static void GetMediumFromObject(object data, out STGMEDIUM medium)
    {
      // We'll serialize to a managed stream temporarily.
      MemoryStream stream = new MemoryStream();

      // Use a custom identifier to mark our data.
      stream.Write(ManagedDataStamp.ToByteArray(), 0, Marshal.SizeOf(typeof(Guid)));

      BinaryFormatter formatter = new BinaryFormatter();
      formatter.Serialize(stream, data.GetType());
      formatter.Serialize(stream, GetAsSerializable(data));

      // Now copy to an HGLOBAL
      byte[] bytes = stream.GetBuffer();
      IntPtr p = Marshal.AllocHGlobal(bytes.Length);
      try
      {
        Marshal.Copy(bytes, 0, p, bytes.Length);
      }
      catch
      {
        Marshal.FreeHGlobal(p);
        throw;
      }

      medium.unionmember = p;
      medium.tymed = TYMED.TYMED_HGLOBAL;
      medium.pUnkForRelease = null;
    }

  }

  #endregion
  
  #region DataObject Implementation

  /// <summary>
  /// Implementation of an own DataObject as extended alternative to the standard DataObject to allow
  /// setting unknown data formats (needed in drag/drop operations).
  /// </summary>
  [ComVisible(true)]
  public class DataObject : IDataObject, IDisposable
  {
    // Internal storage.
    private IList<KeyValuePair<FORMATETC, STGMEDIUM>> storage;

    private int nextConnectionId = 1;

    // List of advisory connections.
    private IDictionary<int, AdviseEntry> connections;

    // Represents an advisory connection entry.
    private class AdviseEntry
    {
      public FORMATETC format;
      public ADVF advf;
      public IAdviseSink sink;

      public AdviseEntry(ref FORMATETC format, ADVF advf, IAdviseSink sink)
      {
        this.format = format;
        this.advf = advf;
        this.sink = sink;
      }
    }

    public DataObject()
    {
      storage = new List<KeyValuePair<FORMATETC, STGMEDIUM>>();
      connections = new Dictionary<int, AdviseEntry>();
    }

    ~DataObject()
    {
      Dispose(false);
    }

    private void ClearStorage()
    {
      foreach (KeyValuePair<FORMATETC, STGMEDIUM> pair in storage)
      {
        STGMEDIUM medium = pair.Value;
        Win32.ReleaseStgMedium(ref medium);
      }
      storage.Clear();
    }

    public void Dispose()
    {
      Dispose(true);
    }

    private void Dispose(bool disposing)
    {
      ClearStorage();
    }

    /// <summary>
    /// Helper method to copy a storage medium.
    /// </summary>
    private STGMEDIUM CopyMedium(ref STGMEDIUM source)
    {
      STGMEDIUM copy = new STGMEDIUM();
      int result = Win32.CopyStgMedium(ref source, ref copy);
      if (result != Win32.S_OK)
        throw Marshal.GetExceptionForHR(result);

      return copy;
    }

    // IDataObject implementation.

    private bool GetDataEntry(ref FORMATETC pFormatetc, out KeyValuePair<FORMATETC, STGMEDIUM> dataEntry)
    {
      foreach (KeyValuePair<FORMATETC, STGMEDIUM> entry in storage)
      {
        FORMATETC format = entry.Key;
        if (IsFormatCompatible(ref pFormatetc, ref format))
        {
          dataEntry = entry;
          return true;
        }
      }

      // Not found... default allocate the out param
      dataEntry = default(KeyValuePair<FORMATETC, STGMEDIUM>);
      return false;
    }

    /// <summary>
    /// Raises the DataChanged event for the specified connection.
    /// </summary>
    /// <param name="connection">The connection id.</param>
    /// <param name="dataEntry">The data entry for which to raise the event.</param>
    private void RaiseDataChanged(int connection, ref KeyValuePair<FORMATETC, STGMEDIUM> dataEntry)
    {
      AdviseEntry adviseEntry = connections[connection];
      FORMATETC format = dataEntry.Key;
      STGMEDIUM medium;
      if ((adviseEntry.advf & ADVF.ADVF_NODATA) != ADVF.ADVF_NODATA)
        medium = dataEntry.Value;
      else
        medium = default(STGMEDIUM);

      adviseEntry.sink.OnDataChange(ref format, ref medium);

      if ((adviseEntry.advf & ADVF.ADVF_ONLYONCE) == ADVF.ADVF_ONLYONCE)
        connections.Remove(connection);
    }

    /// <summary>
    /// Raises the DataChanged event for any advisory connections that
    /// are listening for it.
    /// </summary>
    /// <param name="dataEntry">The relevant data entry.</param>
    private void RaiseDataChanged(ref KeyValuePair<FORMATETC, STGMEDIUM> dataEntry)
    {
      foreach (KeyValuePair<int, AdviseEntry> connection in connections)
      {
        if (IsFormatCompatible(connection.Value.format, dataEntry.Key))
          RaiseDataChanged(connection.Key, ref dataEntry);
      }
    }

    private bool IsFormatCompatible(FORMATETC format1, FORMATETC format2)
    {
      return IsFormatCompatible(ref format1, ref format2);
    }

    private bool IsFormatCompatible(ref FORMATETC format1, ref FORMATETC format2)
    {
      return ((format1.tymed & format2.tymed) > 0
              && format1.dwAspect == format2.dwAspect
              && format1.cfFormat == format2.cfFormat);
    }
    
    public int DAdvise(ref FORMATETC pFormatetc, ADVF advf, IAdviseSink adviseSink, out int connection)
    {
      // Check that the specified advisory flags are supported.
      const ADVF ADVF_ALLOWED = ADVF.ADVF_NODATA | ADVF.ADVF_ONLYONCE | ADVF.ADVF_PRIMEFIRST;
      if ((int)((advf | ADVF_ALLOWED) ^ ADVF_ALLOWED) != 0)
      {
        connection = 0;
        return Win32.OLE_E_ADVISENOTSUPPORTED;
      }

      // Create and insert an entry for the connection list
      AdviseEntry entry = new AdviseEntry(ref pFormatetc, advf, adviseSink);
      connections.Add(nextConnectionId, entry);
      connection = nextConnectionId;
      nextConnectionId++;

      // If the ADVF_PRIMEFIRST flag is specified and the data exists,
      // raise the DataChanged event now.
      if ((advf & ADVF.ADVF_PRIMEFIRST) == ADVF.ADVF_PRIMEFIRST)
      {
        KeyValuePair<FORMATETC, STGMEDIUM> dataEntry;
        if (GetDataEntry(ref pFormatetc, out dataEntry))
          RaiseDataChanged(connection, ref dataEntry);
      }

      return Win32.S_OK;
    }

    public void DUnadvise(int connection)
    {
      connections.Remove(connection);
    }

    public int EnumDAdvise(out IEnumSTATDATA enumAdvise)
    {
      throw Marshal.GetExceptionForHR(Win32.OLE_E_ADVISENOTSUPPORTED);
    }

    public int GetCanonicalFormatEtc(ref FORMATETC formatIn, out FORMATETC formatOut)
    {
      formatOut = formatIn;
      return Win32.DV_E_FORMATETC;
    }

    public IEnumFORMATETC EnumFormatEtc(DATADIR direction)
    {
      // We only support GET.
      if (DATADIR.DATADIR_GET == direction)
        return new EnumFORMATETC(storage);

      throw new NotImplementedException("DataObject: only \"get\" data direction supported in EnumFormatEtc");
    }

    public void GetData(ref FORMATETC format, out STGMEDIUM medium)
    {
      medium = new STGMEDIUM();
      GetDataHere(ref format, ref medium);
    }

    public void GetDataHere(ref FORMATETC format, ref STGMEDIUM medium)
    {
      foreach (KeyValuePair<FORMATETC, STGMEDIUM> pair in storage)
      {
        if ((pair.Key.tymed & format.tymed) > 0 &&
          pair.Key.dwAspect == format.dwAspect && pair.Key.cfFormat == format.cfFormat)
        {
          STGMEDIUM source = pair.Value;
          medium = CopyMedium(ref source);
          return;
        }
      }
      
      // The managed IDataObject definitions (both from Windows.Forms and ComTypes) do not allow
      // returning an error code as the underlying COM interface does. Hence we cannot return
      // a code in an error case. Raising an exception however crashes unnecessarily.
      // So for now we return an empty medium (seems to work so far).
     // throw Marshal.GetExceptionForHR(Win32.DV_E_FORMATETC);
    }

    public int QueryGetData(ref FORMATETC format)
    {
      if ((DVASPECT.DVASPECT_CONTENT & format.dwAspect) == 0)
        return Win32.DV_E_DVASPECT;

      int ret = Win32.DV_E_TYMED;

      foreach (KeyValuePair<FORMATETC, STGMEDIUM> pair in storage)
      {
        if ((pair.Key.tymed & format.tymed) > 0)
        {
          if (pair.Key.cfFormat == format.cfFormat)
            return Win32.S_OK; // Everything ok.
          else
            ret = Win32.DV_E_CLIPFORMAT; // Medium found but in wrong format.
        }
        else
          ret = Win32.DV_E_TYMED; // Wrong medium type.
      }

      return ret;
    }

    public void SetData(ref FORMATETC formatIn, ref STGMEDIUM medium, bool release)
    {
      // First remove existing data.
      foreach (KeyValuePair<FORMATETC, STGMEDIUM> pair in storage)
      {
        if ((pair.Key.tymed & formatIn.tymed) > 0
            && pair.Key.dwAspect == formatIn.dwAspect && pair.Key.cfFormat == formatIn.cfFormat)
        {
          storage.Remove(pair);
          break;
        }
      }

      // Parameter release indicates if we should take owner ship of the data
      // (int which case we just keep the reference) otherwise we copy it, so the caller can free
      // the original data whenever it sees fit.
      STGMEDIUM target = medium;
      if (!release)
        target = CopyMedium(ref medium);

      KeyValuePair<FORMATETC, STGMEDIUM> addPair = new KeyValuePair<FORMATETC, STGMEDIUM>(formatIn, target);
      storage.Add(addPair);
    }
    
    #region EnumFORMATETC Implementation

    /// <summary>
    /// Implementation of the IEnumFORMATETC interface for our DataObject.
    /// </summary>
    [ComVisible(true)]
    private class EnumFORMATETC : IEnumFORMATETC
    {
      private FORMATETC[] formats;
      private int currentIndex = 0;

      internal EnumFORMATETC(IList<KeyValuePair<FORMATETC, STGMEDIUM> > storage)
      {
        formats = new FORMATETC[storage.Count];
        for (int i = 0; i < formats.Length; i++)
          formats[i] = storage[i].Key;
      }

      private EnumFORMATETC(FORMATETC[] formats)
      {
        this.formats = new FORMATETC[formats.Length];
        formats.CopyTo(this.formats, 0);
      }

      public void Clone(out IEnumFORMATETC newEnum)
      {
        EnumFORMATETC ret = new EnumFORMATETC(formats);
        ret.currentIndex = currentIndex;
        newEnum = ret;
      }

      public int Next(int celt, FORMATETC[] rgelt, int[] pceltFetched)
      {
        if (pceltFetched != null && pceltFetched.Length > 0)
          pceltFetched[0] = 0;

        if (celt <= 0 || rgelt == null || currentIndex >= formats.Length)
          return Win32.S_FALSE;

        if ((pceltFetched == null || pceltFetched.Length < 1) && celt != 1)
          return Win32.S_FALSE;

        if (rgelt.Length < celt)
          throw new ArgumentException("The number of elements in the return array is less than the number of elements requested");

        int remaining = celt;
        for (int i = 0; currentIndex < formats.Length && remaining > 0; i++, remaining--, currentIndex++)
          rgelt[i] = formats[currentIndex];

        if (pceltFetched != null && pceltFetched.Length > 0)
          pceltFetched[0] = celt - remaining;

        return (remaining == 0) ? Win32.S_OK : Win32.S_FALSE;
      }

      public int Reset()
      {
        currentIndex = 0;
        return Win32.S_OK;
      }

      public int Skip(int celt)
      {
        if (currentIndex + celt > formats.Length)
          return Win32.S_FALSE;

        currentIndex += celt;
        return Win32.S_OK;
      }

    }

    #endregion
  }

  #endregion // DataObject

  #region IDragSourceHelper

  [ComVisible(true)]
  [ComImport]
  [Guid("DE5BF786-477A-11D2-839D-00C04FD918D0")]
  [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
  public interface IDragSourceHelper
  {
    void InitializeFromBitmap(
        [In, MarshalAs(UnmanagedType.Struct)] ref Win32.ShDragImage dragImage,
        [In, MarshalAs(UnmanagedType.Interface)] IDataObject dataObject);

    void InitializeFromWindow(
        [In] IntPtr hwnd,
        [In] ref Win32.POINT pt,
        [In, MarshalAs(UnmanagedType.Interface)] IDataObject dataObject);
  }

  #endregion // IDragSourceHelper

  #region IDropTargetHelper

  [ComVisible(true)]
  [ComImport]
  [Guid("4657278B-411B-11D2-839A-00C04FD918D0")]
  [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
  public interface IDropTargetHelper
  {
    void DragEnter(
        [In] IntPtr hwndTarget,
        [In, MarshalAs(UnmanagedType.Interface)] IDataObject dataObject,
        [In] ref Win32.POINT pt,
        [In] int effect);

    void DragLeave();

    void DragOver(
        [In] ref Win32.POINT pt,
        [In] int effect);

    void Drop(
        [In, MarshalAs(UnmanagedType.Interface)] IDataObject dataObject,
        [In] ref Win32.POINT pt,
        [In] int effect);

    void Show(
        [In] bool show);
  }

  #endregion // IDropTargetHelper

  #region DragDropHelper

  [ComImport]
  [Guid("4657278A-411B-11d2-839A-00C04FD918D0")]
  public class DragDropHelper { }

  #endregion // DragDropHelper
}
