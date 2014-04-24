Option Explicit

' © 2009 Sun Microsystems, Inc.
'
' This library is free software; you can redistribute it and/or
' modify it under the terms of the GNU Lesser General Public
' License as published by the Free Software Foundation; either
' version 2 of the License, or (at your option) any later version.
'
' This library is distributed in the hope that it will be useful,
' but WITHOUT ANY WARRANTY; without even the implied warranty of
' MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
' Lesser General Public License for more details.
'
' You should have received a copy of the GNU Lesser General Public
' License along with this library; if not, write to the
' Free Software Foundation, Inc., 59 Temple Place - Suite 330,
' Boston, MA 02111-1307, USA.

' Purpose:
'   VBScript that prints system status to stdout - reduced version
'
' Usage:
'   cscript //NoLogo res\scripts\shell\mysql_system_status.vbs /LoopCount:30 /Interval:1000
'   or
'   cscript //NoLogo res\scripts\shell\mysql_system_status.vbs /DoStdIn
'
' Notes:
'   Please note that CScript has to be used.

' -----------------------------------------------------------------------------
' Dim variables
Dim oFso, oWmiService, oItems, oItem, oStdOut, oStdIn, sCmd, sCpu, sMem, doStdIn
Dim oPhysMemItems, physMem, TotalMem, availMem

' -----------------------------------------------------------------------------
' Init Objects
Set oFso = CreateObject("Scripting.FileSystemObject")
Set oWmiService = GetObject("winmgmts:\\.\root\CIMV2")
Set oStdOut = oFso.GetStandardStream(1)
Set oStdIn = oFso.GetStandardStream(0)

TotalMem = 0
Set oPhysMemItems = oWmiService.InstancesOf("Win32_OperatingSystem")
oStdOut.WriteLine("1")
For Each physMem in oPhysMemItems
  TotalMem = physMem.TotalVisibleMemorySize
Next

' -----------------------------------------------------------------------------
' Loop as long as required
While (1 = 1)
	' Reset vars
	sCpu= "C "
	sMem= "M "
	Set oItems = oWmiService.ExecQuery("SELECT * FROM Win32_PerfFormattedData_PerfOS_Processor WHERE Name = '_Total'") 
	For Each oItem In oItems
		sCpu= sCpu + CStr(oItem.PercentProcessorTime)
	Next
	oStdOut.WriteLine(sCpu)
	Set oItems = oWmiService.ExecQuery("Select * FROM Win32_PerfFormattedData_PerfOS_Memory")
    availMem = 0
	For Each oItem In oItems
		availMem= availMem + oItem.AvailableKBytes
	Next
	oStdOut.WriteLine(sMem + CStr((TotalMem - availMem)/TotalMem * 100))
	WScript.sleep(2000)
Wend

' Exit
WScript.Quit
