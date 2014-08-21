
import grt

# NOTE: If you're using this in a module, it should be imported as 
# from wb import *


class DefineModule(dict):
    def __init__(self, name, implements=None, author="", version= "", description= ""):
        """Define a GRT module. Must be called before any function declaration as
        wbmodule = DefineModule('modname')
        """

        self.name= name
        self.author= author
        self.version= version
        self.description= description
    
        # List of functions exported by the module (automatically updated by @declare)
        self.functions= []
        # List of interfaces implemented by the module
        self.implements= implements or []
    
        self._pluginList= grt.List(grt.OBJECT, "app.Plugin")

        
        
    def __getitem__(self, name):
        return getattr(self, name)


    @property
    def moduleDataDirectory(self):
      return grt.root.wb.registry.appDataDirectory+"/modules/data"


    ##
    ## Decorators for Writing Modules and Plugins
    ##
    def plugin(self, name, caption= "", description="", type="standalone", input= [], groups= [], pluginMenu= None):
        """Decorator to declare a Plugin, used in addition to @wbexport
        Usage:
        @wbmodule.plugin("db.utils.mangleNames", caption="Mangle Names", description="Mangles all object names in current catalog beyond recognition.", input= [wbinputs.currentCatalog()], groups=["Menu/Catalog"])
        @wbmodule.export(grt.INT, grt.classes.db_Catalog)
        def mangleNames(catalog):
           return 1
        """
        
        def setup_plugin(fn):    
            # make sure getPluginInfo() is in the function list
            if "getPluginInfo" not in [x[0] for x in self.functions]:
              self.functions.append(("getPluginInfo", 
                                              ((grt.LIST, (grt.OBJECT, "app.Plugin")),
                                              []),
                                              lambda: self._pluginList))
              if "PluginInterface" not in self.implements:
                  self.implements.append("PluginInterface")
            
            plug= grt.classes.app_Plugin()
            plug.name= name
            plug.caption= caption
            plug.description= description
            plug.pluginType= type
            plug.moduleName= self.name
            plug.moduleFunctionName= fn.func_code.co_name
            for i in input:
              i.owner= plug
              plug.inputValues.append(i)
            for g in groups:
              plug.groups.append(g)
            if pluginMenu:
              plug.groups.append("Menu/"+pluginMenu)
            plug.rating= 100
            plug.showProgress= 0
            self._pluginList.append(plug)
            
            return fn
        
        return setup_plugin

    def exportFilter(self, name, caption="", input="selectedText"):
        def setup_plugin(fn):    
            # make sure getPluginInfo() is in the function list
            if "getPluginInfo" not in [x[0] for x in self.functions]:
              self.functions.append(("getPluginInfo", 
                                              ((grt.LIST, (grt.OBJECT, "app.Plugin")),
                                              []),
                                              lambda: self._pluginList))
              if "PluginInterface" not in self.implements:
                  self.implements.append("PluginInterface")
            
            plug= grt.classes.app_Plugin()
            plug.name= name
            plug.caption= caption
            plug.pluginType= "standalone"
            plug.moduleName= self.name
            plug.moduleFunctionName= fn.func_code.co_name
            if input:
                arg = grt.classes.app_PluginInputDefinition()
                arg.name= input
                plug.inputValues.append(arg)
            plug.groups.append("Filter")
            plug.rating= 100
            plug.showProgress= 0
            self._pluginList.append(plug)

            signature= (grt.STRING, [("text", grt.STRING)])
            self.functions.append((fn.func_code.co_name, signature, fn))

            return fn

        return setup_plugin

       
    
    
    def export(self, returntype, *argtypes):
        """Decorator to declare an exported Module function.
        Usage:
        @wbmodule.export(grt.INT, grt.classes.db_Table, (grt.LIST, grt.STRING))
        def dostuff(arg1, arg2):
          return 1
        
        Declares the function dostuff as returning an integer and having the 1st arg a db.Table and
        the 2nd a list of strings.
        """
        typenames= [grt.INT,grt.DOUBLE,grt.STRING,grt.LIST,grt.DICT, grt.OBJECT]
        def set_types(fn):
            if len(argtypes) != fn.func_code.co_argcount:
              raise TypeError("module function '%s' has %i arguments, but @export declares %i" % (fn.func_code.co_name, fn.func_code.co_argcount, len(argtypes)))
            arglist= []
            for i in range(len(argtypes)+1):
              if i == 0:
                arg = returntype
              else:
                arg= argtypes[i-1]
              
              if arg == grt.List:
                  arg = grt.LIST
              elif arg == grt.Dict:
                  arg = grt.DICT
              
              if type(arg) == tuple:
                containertype, contenttype= arg
                if containertype not in [grt.LIST, grt.DICT]:
                  raise TypeError("argument %i has invalid specification (type %s is not a container type and takes no extra argument)"%(i, containertype))
                
                if contenttype in dir(grt.classes) or (getattr(contenttype, "__name__", None) in dir(grt.classes)):
                  contenttype= (grt.OBJECT, contenttype if type(contenttype) is str else contenttype.__name__.replace("_", "."))
                elif contenttype not in typenames:
                  raise TypeError("argument %i has invalid specification (%s it not a valid content type or class)"%(i, contenttype))

                arg= (containertype, contenttype)
              elif arg in (grt.INT, grt.DOUBLE, grt.STRING, grt.LIST, grt.DICT, grt.OBJECT):
                pass                
              elif (type(arg) == str and arg not in typenames) and arg not in grt.classes:
                raise TypeError("%s not a valid GRT type specification"%str(arg))
              else:
                arg= (grt.OBJECT, arg.__name__.replace("_", "."))
            
              if i == 0:
                arglist.append(arg)
              else:
                arglist.append((fn.func_code.co_varnames[i-1], arg))
            signature= (arglist[0], arglist[1:])
        
            self.functions.append((fn.func_code.co_name, signature, fn))
            return fn
        return set_types
 


#def SimplePlugin(name, author="", version="", caption="", input=[], returns=grt.INT):
#    def auto_wrap(fn):
#        global ModuleInfo
#        ModuleInfo = DefineModule(name, author=author, version=version)
#        exp = ModuleInfo.export(returns, *input)
#        pl = ModuleInfo.plugin(name+"."+fn.name, caption=caption if caption else fn.name)
#        return pl(exp(fn))
#    return auto_wrap
  

#
# Plugin input type helpers and predefined types.
#


class _wbinputs:
  def objectOfClass(self, className):
    assert type(className) == str
    arg= grt.classes.app_PluginObjectInput()
    arg.objectStructName= className
    return arg


  def string(self):
    arg= grt.classes.app_PluginInputDefinition()
    arg.name= "string"
    return arg

  # Home
  def selectedConnection(self):
    arg= grt.classes.app_PluginObjectInput() 
    arg.name= "selectedConnection"
    arg.objectStructName= "db.mgmt.Connection"
    return arg

  def selectedInstance(self):
    arg= grt.classes.app_PluginObjectInput() 
    arg.name= "selectedInstance"
    arg.objectStructName= "db.mgmt.ServerInstance"
    return arg



  # Modeling
  def currentModel(self):
    arg= grt.classes.app_PluginObjectInput() 
    arg.name= "activeModel"
    arg.objectStructName= "workbench.physical.Model"
    return arg

  def currentCatalog(self):
    arg= grt.classes.app_PluginObjectInput() 
    arg.name= "activeCatalog"
    arg.objectStructName= "db.Catalog"
    return arg
    
  def currentDiagram(self):
    arg= grt.classes.app_PluginObjectInput()
    arg.name= "activeDiagram"
    arg.objectStructName= "workbench.physical.Diagram"
    return arg

  def selectedDiagram(self):
    arg= grt.classes.app_PluginObjectInput()
    arg.name= ""
    arg.objectStructName= "workbench.physical.Diagram"
    return arg

  # SQL Editor
  def currentSQLEditor(self):
    arg= grt.classes.app_PluginObjectInput()
    arg.name= "activeSQLEditor"
    arg.objectStructName= "db.query.Editor"
    return arg
    

  def currentQueryEditor(self):
    arg= grt.classes.app_PluginObjectInput()
    arg.name= "activeQueryEditor"
    arg.objectStructName= "db.query.QueryEditor"
    return arg
  currentQueryBuffer = currentQueryEditor
    
  def currentResultset(self):
    arg= grt.classes.app_PluginObjectInput()
    arg.name= "activeResultset"
    arg.objectStructName= "db.query.Resultset"
    return arg

  def currentEditableResultset(self):
    arg= grt.classes.app_PluginObjectInput()
    arg.name= "activeResultset"
    arg.objectStructName= "db.query.EditableResultset"
    return arg

  def selectedLiveObject(self):
    arg= grt.classes.app_PluginObjectInput()
    arg.name= "" # any
    arg.objectStructName= "db.query.LiveDBObject"
    return arg

  def selectedLiveSchema(self):
    arg= grt.classes.app_PluginObjectInput()
    arg.name= "schema"
    arg.objectStructName= "db.query.LiveDBObject"
    return arg

  def selectedLiveTable(self):
    arg= grt.classes.app_PluginObjectInput()
    arg.name= "table"
    arg.objectStructName= "db.query.LiveDBObject"
    return arg

  def selectedLiveView(self):
    arg= grt.classes.app_PluginObjectInput()
    arg.name="view"
    arg.objectStructName= "db.query.LiveDBObject"
    return arg

  def selectedLiveRoutine(self):
    arg= grt.classes.app_PluginObjectInput()
    arg.name="routine"
    arg.objectStructName= "db.query.LiveDBObject"
    return arg

  def selectedRowList(self):
    arg= grt.classes.app_PluginInputDefinition()
    arg.name= "selectedRowList"
    return arg

  def clickedRow(self):
    arg= grt.classes.app_PluginInputDefinition()
    arg.name= "clickedRow"
    return arg

  def clickedColumn(self):
    arg= grt.classes.app_PluginInputDefinition()
    arg.name= "clickedColumn"
    return arg

  def simpleValue(self, name):
      arg= grt.classes.app_PluginInputDefinition()
      arg.name= name
      return arg



wbinputs= _wbinputs()

