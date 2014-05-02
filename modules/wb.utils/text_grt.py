# import the wb module
from wb import *
# import the grt module
import grt
# import the mforms module for GUI stuff
import mforms



# define this Python module as a GRT module
ModuleInfo = DefineModule(name= "TextUtils", author= "Oracle Corp.", version="1.0")




#@ModuleInfo.plugin("wb.text.doStuff", caption= "Do Stuff", input= [wbinputs.currentQueryBuffer()], pluginMenu= "Text")
#@ModuleInfo.export(grt.INT, grt.classes.db_query_QueryBuffer)
#def doStuff(qbuffer):
#
#  return 0


@ModuleInfo.exportFilter("wb.text.sort", "Sort Selection")
def sortText(text):
  lines = text.split("\n")
  lines.sort()
  return "\n".join(lines)


@ModuleInfo.exportFilter("wb.text.quoteString", "Quote and Escape String")
def quoteString(text):
  escaped= ""
  for c in text:
    escape= None
    if ord(c) == 0:
      escape= '0'
    elif c == '\n':
      escape= 'n'
    elif c == '\r':
      escape= 'r'
    elif c == '\\':
      escape= '\\'
    elif c == "'":
      escape= "'"
    elif ord(c) == 032:
      escape= 'Z' 

    if escape is not None:
      escaped += "\\"+escape
    else:
      escaped += c
  return "'"+escaped+"'"



